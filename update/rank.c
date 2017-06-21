/*
 * Let's talk about relational databases and ranking.
 *
 * Given a player, it is easy to compute it's rank.  One could do a
 * self-join on the players table, or one could count the number of
 * players with a higher elo score.  Both are common and I couldn't
 * find any better way, at least using what SQlite has to offer.
 *
 * Those methods are not without issues tho.  They still require a full
 * database scanning to compute rank.  Hence a O(n) complexity.  For
 * instance, ranking 100 players in a database with 50 000 players took
 * 2 seconds.
 *
 * This leads to the the fact that we cant really compute ranks on the
 * fly.  We have to precompute it at some point.  The first attempts
 * showed that rank calculation is quite an expensive process: we can't
 * do it for each database insert/update.
 *
 * For instance, computing rank of 50 000 players took 1 second on my
 * computer.  Not that slow, but slow enough so that it's not something
 * we can afford at every database update.
 *
 * hence we had to do it at regular intervals.  It's quite sad that we
 * came back to this method and gave up on the real time ranking.  But
 * on the other side it makes pages load much faster, and allow to scale
 * past 50 000 players.
 *
 * There is something not trivial when delaying ranks calculation: elo
 * scores and rank has to be consistent for the end user.  Wich means
 * that if we delay rank calculation but not elo scores, it could
 * results in inconsistencies where you have a better rank than someone
 * who have more elo points, and vice-versa.
 *
 * So we also need to wait for the rank calculation process before
 * writing new elo scores in the database.  What we do is to store new
 * elo scores in a special table, and then the rank calculation process
 * will flush this table at the same time it writes ranks.
 *
 * Last but not least, we record ranks per maps, per gametypes, as well
 * as a global rank, wich is like a combination of every maps for every
 * gametype.  You will encounter an array with 3 entries while reading
 * the code, that is to store temporary values for each of these ranking
 * categories.
 *
 * I'm not really familiar with relational databases yet so I could have
 * missed an obvious solution.  For now, this seems to work better than
 * everything else I tried.  Fingers crossed.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include "teerank.h"
#include "database.h"
#include "rank.h"
#include "server.h"

/* Contains necessarry info to rank a player */
struct player_info {
	char name[NAME_STRSIZE];

	struct {
		int elo;
		unsigned rank;
	} gametype, map;

	int is_rankable;
	struct client *old, *new;
};

/*
 * We carry a player array through those functions, but we actually
 * never carry the array length.  Yet we have all info to know when we
 * reached the end or when a player is not loaded.  Its quite long and
 * verbose so put it in a macro for convenience.
 */
#define foreach_player_info(p) \
	for (i = 0, p = players; i < MAX_CLIENTS; i++, p++) \
		if (!p->new); else

static struct client *find_client(struct server *server, const char *pname)
{
	unsigned i;

	for (i = 0; i < server->num_clients; i++)
		if (strcmp(server->clients[i].name, pname) == 0)
			return &server->clients[i];

	return NULL;
}

static int is_already_loaded(struct player_info *players, const char *pname)
{
	unsigned i;
	struct player_info *p;

	foreach_player_info(p)
		if (strcmp(p->name, pname) == 0)
			return 1;

	return 0;
}

static void read_elo(struct sqlite3_stmt *res, void *_elo)
{
	int *elo = _elo;
	*elo = sqlite3_column_int(res, 0);
}

/*
 * When an elo score cannot be loaded for some reasons, use this special
 * value to skip the player for this ranking session.
 */
static const int ERROR_NO_ELO = INT_MIN;

/*
 * When ranking players, we need the latest score available.  Taking the
 * score from the "ranks" table is not enough because there could be
 * newer elo waiting in "pending".  So query "pending" first, and then
 * "ranks".
 */
static int latest_elo(const char *pname, const char *gametype, const char *map)
{
	unsigned nrow;
	sqlite3_stmt *res;
	int elo;

	const char *in_pending =
		"SELECT elo"
		" FROM pending"
		" WHERE name = ? AND gametype = ? AND map = ?";

	const char *in_ranks =
		"SELECT elo"
		" FROM ranks"
		" WHERE name = ? AND gametype = ? AND map = ?";

	foreach_row(in_pending, read_elo, &elo, "sss", pname, gametype, map);

	if (res && nrow)
		return elo;

	foreach_row(in_ranks, read_elo, &elo, "sss", pname, gametype, map);

	/*
	 * We don't return DEFAULT_ELO on error, because we don't want
	 * to overide the old elo score.
	 */
	if (res && nrow)
		return elo;
	else if (res)
		return DEFAULT_ELO;
	else
		return ERROR_NO_ELO;
}

/* Load every unique players and query their latest elo score */
static void load_players(struct server *old, struct server *new, struct player_info *players)
{
	struct player_info *p = players;
	unsigned i;
	char *pname;

	for (i = 0; i < new->num_clients; i++) {
		pname = new->clients[i].name;

		/* Avoid duplicates */
		if (is_already_loaded(players, pname))
			continue;

		p->gametype.elo = latest_elo(pname, new->gametype, NULL);
		p->map.elo      = latest_elo(pname, new->gametype, new->map);

		/* If an elo could not be acquired, ignore the player */
		if (
			p->gametype.elo == ERROR_NO_ELO ||
			p->map.elo      == ERROR_NO_ELO)
			continue;

		snprintf(p->name, sizeof(p->name), "%s", pname);
		p->new = &new->clients[i];
		p->old = find_client(old, pname);

		p++;
	}
}

static time_t get_elapsed_time(struct server *old, struct server *new)
{
	if (old->lastseen > new->lastseen)
		return 0;
	else
		return new->lastseen - old->lastseen;
}

/*
 * Detecting when 'old' and 'new' are two different games is not
 * straightforward.  We have to rely on heuristics.  It is a new game if
 * the difference between old score average and new score average is
 * greater than 3.
 */
static int is_new_game(struct player_info *players)
{
	struct player_info *p;
	unsigned i, nr_players = 0;
	int oldtotal = 0, newtotal = 0;

	foreach_player_info(p) {
		if (p->old && p->new) {
			oldtotal += p->old->score;
			newtotal += p->new->score;
			nr_players++;
		}
	}

	if (!nr_players)
		return 0;

	float oldavg = oldtotal / nr_players;
	float newavg = newtotal / nr_players;

	return oldavg - newavg > 3.0;
}

static void mark_rankable_players(
	struct server *old, struct server *new, struct player_info *players)
{
	unsigned i, rankable = 0;
	struct player_info *p;
	time_t elapsed;

	assert(players != NULL);

	/* New games don't have a meaningful "old" state to rank against */
	if (is_new_game(players))
		goto dont_rank;

	/*
	 * Map and gametype should not have changed, otherwise it's like
	 * a new game.
	 */
	if (
		strcmp(new->gametype, old->gametype) != 0 ||
		strcmp(new->map, old->map) != 0)
		goto dont_rank;

	elapsed = get_elapsed_time(old, new);

	/*
	 * 30 minutes between each update is just too much and it increase
	 * the chance of rating two different games.
	 */
	if (elapsed > 30 * 60)
		goto dont_rank;

	/*
	 * On the other hand, less than 1 minutes between updates is
	 * also meaningless.
	 */
	if (elapsed < 60)
		goto dont_rank;

	/* Mark rankable players */
	foreach_player_info(p) {
		if (p->old && p->new->ingame) {
			p->is_rankable = 1;
			rankable++;
		}
	}

	/*
	 * We don't rank games with less than 4 rankable players.  We believe
	 * it is too much volatile to rank those kind of games.
	 */
	if (rankable < 4)
		goto dont_rank;

	return;

dont_rank:
	foreach_player_info(p)
		p->is_rankable = 0;
}

/* p() func as defined by Elo. */
static double p(double delta)
{
	if (delta > 400.0)
		delta = 400.0;
	else if (delta < -400.0)
		delta = -400.0;

	return 1.0 / (1.0 + pow(10.0, -delta / 400.0));
}

/* Classic Elo formula for two players */
static void compute_elo_delta(struct player_info *p1, struct player_info *p2, int *delta)
{
	const unsigned K = 25;
	int d1, d2;
	double W;

	assert(p1 != NULL);
	assert(p2 != NULL);
	assert(p1 != p2);

	d1 = p1->new->score - p1->old->score;
	d2 = p2->new->score - p2->old->score;

	if (d1 < d2)
		W = 0.0;
	else if (d1 == d2)
		W = 0.5;
	else
		W = 1.0;

	delta[0] = K * (W - p(p1->gametype.elo - p2->gametype.elo));
	delta[1] = K * (W - p(p1->map.elo      - p2->map.elo));
}

/*
 * Elo has been designed for two players games only.  In order to have
 * meaningul value for multiplayer, we have to modify how new Elos are
 * computed.
 *
 * To get the new Elo for a player, we match this player against every
 * other players and we make the average of every Elo deltas.  The Elo
 * delta is then added to the player's Elo points.
 */
static void compute_new_elo(struct player_info *player, struct player_info *players, int *elo)
{
	struct player_info *p;
	unsigned i;

	int count = 0;
	int delta[2], total[2] = { 0 };

	assert(player != NULL);
	assert(players != NULL);
	assert(elo != NULL);

	foreach_player_info(p) {
		if (p != player && p->is_rankable) {
			compute_elo_delta(player, p, delta);
			total[0] += delta[0];
			total[1] += delta[1];
			count++;
		}
	}

	elo[0] = player->gametype.elo + total[0] / count;
	elo[1] = player->map.elo      + total[1] / count;
}

static void verbose_elo_update(struct player_info *p, int *newelo)
{
	verbose(
		"\t%-16s | %4d | %4d %+3d | %4d %+3d",
		p->name, p->new->score - p->old->score,
		newelo[0], newelo[0] - p->gametype.elo,
		newelo[1], newelo[1] - p->map.elo);
}

/*
 * This does calculate new elos and store them in the "pending" table.
 * To actually make them visible to the end user, one have to call
 * recompute_ranks().
 */
static void update_elos(
	struct player_info *players, const char *gametype,
	const char *map, const char *ip, const char *port)
{
	struct player_info *p;
	unsigned i;
	int elo[2];
	unsigned ranked = 0;

	const char *query =
		"INSERT OR REPLACE INTO pending VALUES(?, ?, ?, ?)";

	assert(players != NULL);
	assert(gametype != NULL);
	assert(map != NULL);
	assert(ip != NULL);
	assert(port != NULL);

	foreach_player_info(p) {
		if (p->is_rankable)
			ranked++;
	}

	if (ranked)
		verbose("%s:%s: %u players ranked, %s, %s", ip, port, ranked, gametype, map);

	foreach_player_info(p) {
		if (p->is_rankable) {
			compute_new_elo(p, players, elo);
			exec(query, "sssi", p->name, gametype, NULL,  elo[0]);
			exec(query, "sssi", p->name, gametype, map, elo[1]);
			verbose_elo_update(p, elo);
		}
	}
}

void rank_players(struct server *old, struct server *new)
{
	struct player_info players[MAX_CLIENTS] = { 0 };

	load_players(old, new, players);
	mark_rankable_players(old, new, players);
	update_elos(players, new->gametype, new->map, new->ip, new->port);
}

/*
 * Take every pending changes in the "pending" table and definitively
 * commit them in the database.  Ranks will be computed just after so
 * don't bother keeping them.
 */
static void apply_pending_elo(void)
{
	const char *query =
		"INSERT OR REPLACE INTO ranks"
		" (name, gametype, map, elo, rank)"
		"  SELECT p.name, p.gametype, p.map, p.elo, NULL as rank"
		"  FROM pending AS p";

	exec(query);
}

/*
 * For each player with pending change, record their new elo and rank,
 * then flush the pending table.  Elo and ranks should have been
 * computed before calling this function.
 */
static void record_changes(void)
{
	/* Turns out we can do this in a single query */
	const char *query =
		"INSERT OR REPLACE INTO ranks_historic"
		" (name, ts, gametype, map, elo, rank)"
		"  SELECT ranks.name, time('now'), ranks.gametype, ranks.map, ranks.elo, ranks.rank"
		"  FROM pending JOIN ranks"
		"   ON pending.name = ranks.name"
		"    AND pending.gametype = ranks.gametype"
		"    AND pending.map = ranks.map";

	if (exec(query));
		exec("DELETE FROM pending");
}

struct pending {
	const char *name;
	const char *gametype;
	const char *map;
	int elo;
};

static void read_pending(struct sqlite3_stmt *res, void *_p)
{
	struct pending *p = _p;
	p->name = (char*)sqlite3_column_text(res, 0);
	p->gametype = (char*)sqlite3_column_text(res, 1);
	p->map = (char*)sqlite3_column_text(res, 2);
	p->elo = sqlite3_column_int(res, 3);
}

/*
 * This does only compute ranks of player given one gametype and one
 * map.  It is then called multiple times by recompute_ranks() for each
 * gametype and maps that needs ranks to be recomputed.
 */
static void do_recompute_ranks(const char *gametype, const char *map)
{
	unsigned nrow, rank = 0;
	sqlite3_stmt *res;
	struct pending p;

	/* Reuse read_pending() by selecting the same set of fields */
	const char *select =
		"SELECT name, gametype, map, elo"
		" FROM ranks"
		" WHERE gametype = ? AND map = ?"
		" ORDER BY elo DESC, lastseen DESC, name DESC";

	const char *update =
		"UPDATE ranks"
		" SET rank = ?"
		" WHERE name = ? AND gametype = ? AND map = ?";

	foreach_row(select, read_pending, &p, "ss", gametype, map)
		exec(update, "usss", ++rank, p.name, gametype, map);
}

/*
 * This is were we actually recompute ranks of every players in the
 * database.  Not sure yet if it is the faster way to do it.  Verbose
 * time consumed because this is easily the most expensive query in the
 * whole project.
 */
void recompute_ranks(void)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct pending p;
	clock_t clk;

	/*
	 * We don't want to recompute ranks of every possible
	 * combinations of gametype and maps.  That would be way too
	 * expensive.  We only want to update ranks of the combinations
	 * for wich there are changes.
	 *
	 * To query those changes, we can look for gametype and map in
	 * the "pending" table.
	 */
	const char *query =
		"SELECT name, gametype, map, elo"
		" FROM pending"
		" GROUP BY gametype, map";

	clk = clock();

	/*
	 * Drop indices because they will be updated at each update, And
	 * this takes roughly 10% of the total time.  Of course, we
	 * re-create them just after.
	 */
	drop_all_indices();

	/* Commit pending changes in the database */
	apply_pending_elo();

	/* Update ranks */
	foreach_row(query, read_pending, &p)
		do_recompute_ranks(p.gametype, p.map);

	/* Eventually, record new ranks */
	record_changes();

	create_all_indices();

	clk = clock() - clk;
	verbose(
		"Recomputing ranks of %u leagues took %ums",
		nrow, (unsigned)((double)clk / CLOCKS_PER_SEC * 1000.0));
}
