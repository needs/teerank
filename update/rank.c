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
 * I'm not really familiar with relational databases yet so I could have
 * missed an obvious solution.  For now, this seems to work better than
 * everything else I tried.  Fingers crossed.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "teerank.h"
#include "rank.h"
#include "player.h"
#include "database.h"

/*
 * We carry a player array through those functions, but we actually
 * never carry the array length.  Yet we have all info to know when we
 * reached the end or when a player is not loaded.  Its quite long and
 * verbose so put it in a macro for convenience.
 */
#define _foreach_player(p) \
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

static int is_already_loaded(struct player *players, const char *pname)
{
	unsigned i;
	struct player *p;

	_foreach_player(p)
		if (strcmp(p->name, pname) == 0)
			return 1;

	return 0;
}

struct pending {
	const char *name;
	int elo;
};

static void read_pending(struct sqlite3_stmt *res, void *_p)
{
	struct pending *p = _p;
	p->name = (char*)sqlite3_column_text(res, 0);
	p->elo = sqlite3_column_int(res, 1);
}

/* Returns pending elo, if any */
static int get_latest_elo(struct player *player)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct pending pending;

	const char *query =
		"SELECT name, elo"
		" FROM pending"
		" WHERE name = ?";

	foreach_row(query, read_pending, &pending, "s", player->name);
	if (!res || !nrow)
		return player->elo;
	else
		return pending.elo;
}

/*
 * This loads every players in the "new" server.  One subtlety tho: we
 * are gonna rank those players, and for this purpose, we needs the
 * latest elo score available.  That's why we retrieve the elo score
 * from the "pending" table, if any.
 */
static void load_players(struct server *old, struct server *new, struct player *players)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	char *pname;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ?";

	for (i = 0; i < new->num_clients; i++) {
		pname = new->clients[i].name;

		/* Don't load duplicated players */
		if (is_already_loaded(players, pname))
			continue;

		foreach_player(query, players, "s", pname);
		if (!res || !nrow)
			continue;

		players->new = &new->clients[i];
		players->old = find_client(old, pname);
		players->elo = get_latest_elo(players);

		players++;
	}
}

static time_t get_elapsed_time(struct server *old, struct server *new)
{
	if (old->lastseen == NEVER_SEEN)
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
static int is_new_game(struct player *players)
{
	struct player *p;
	unsigned i, nr_players = 0;
	int oldtotal = 0, newtotal = 0;

	_foreach_player(p) {
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
	struct server *old, struct server *new, struct player *players)
{
	unsigned i, rankable = 0;
	struct player *p;
	time_t elapsed;

	assert(players != NULL);

	/* We don't rank non vanilla CTF servers for now */
	if (!is_vanilla_ctf(new->gametype, new->map, new->max_clients))
		goto dont_rank;

	if (is_new_game(players))
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
	_foreach_player(p) {
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
	_foreach_player(p)
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
static int compute_elo_delta(struct player *p1, struct player *p2)
{
	static const unsigned K = 25;
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

	return K * (W - p(p1->elo - p2->elo));
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
static int compute_new_elo(struct player *player, struct player *players)
{
	struct player *p;
	unsigned i;
	int count = 0, total = 0;

	assert(player != NULL);
	assert(players != NULL);

	_foreach_player(p) {
		if (p != player && p->is_rankable) {
			total += compute_elo_delta(player, p);
			count++;
		}
	}

	total = total / count;

	return player->elo + total;
}

static void verbose_elo_updates_header(struct player *players)
{
	int ranked = 0;
	struct player *p;
	unsigned i;

	/* There will be elo updates if there are ranked players */
	_foreach_player(p) {
		if (p->is_rankable)
			ranked++;
	}

	if (ranked)
		verbose("%u players ranked", ranked);
}

static void verbose_elo_update(struct player *p, int newelo)
{
	verbose(
		"\t%-16s | %-16s | %4d | %d -> %d (%+d)",
		p->name, p->clan, p->new->score - p->old->score,
		p->elo, newelo, newelo - p->elo);
}

/*
 * This does calculate new elos and store them in the "pending" table.
 * To actually make them visible to the end user, one have to call
 * recompute_ranks().
 */
static void update_elos(struct player *players)
{
	struct player *p;
	unsigned i;

	const char *query =
		"INSERT OR REPLACE INTO pending VALUES (?, ?)";

	assert(players != NULL);

	verbose_elo_updates_header(players);

	_foreach_player(p) {
		if (p->is_rankable) {
			int elo = compute_new_elo(p, players);
			exec(query, "si", p->name, elo);
			verbose_elo_update(p, elo);
		}
	}
}

void rank_players(struct server *old, struct server *new)
{
	struct player players[MAX_CLIENTS] = { 0 };

	load_players(old, new, players);
	mark_rankable_players(old, new, players);
	update_elos(players);
}

/*
 * Take every pending changes in the "pending" table and definitively
 * commit them in the database.
 */
static void apply_pending_elo(void)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct pending p;

	const char *query =
		"SELECT name, elo"
		" FROM pending";

	foreach_row(query, read_pending, &p)
		exec("UPDATE players SET elo = ? WHERE name = ?", "is", p.elo, p.name);
}

/*
 * For each player with pending change, record their new elo and rank,
 * then flush the pending table.  This does not write new elo in players
 * records, this is done by apply_pending_elo().
 */
static void record_changes(void)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct pending p;

	const char *query =
		"SELECT name, elo"
		" FROM pending";

	foreach_row(query, read_pending, &p)
		record_elo_and_rank(p.name);

	if (res && nrow)
		exec("DELETE FROM pending");
}

static void read_name_and_elo(sqlite3_stmt *res, void *_p)
{
	struct player *p = _p;
	snprintf(p->name, sizeof(p->name), "%s", sqlite3_column_text(res, 0));
	p->elo = sqlite3_column_int(res, 1);
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
	struct player p;
	clock_t clk;

	const char *query =
		"SELECT name, elo"
		" FROM players"
		" ORDER BY" SORT_BY_ELO;

	clk = clock();

	apply_pending_elo();

	/*
	 * Drop indices because they will be updated at each update, And
	 * this takes roughly 10% of the total time.  Of course, we
	 * re-create them just after.
	 */
	drop_all_indices();

	foreach_row(query, read_name_and_elo, &p) {
		p.rank = nrow+1;
		exec("UPDATE players SET rank = ? WHERE name = ?", "us", p.rank, p.name);
	}

	create_all_indices();

	record_changes();

	clk = clock() - clk;
	verbose(
		"Recomputing ranks for %u players took %ums",
		nrow, (unsigned)((double)clk / CLOCKS_PER_SEC * 1000.0));
}
