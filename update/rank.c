#include <math.h>
#include <string.h>
#include <assert.h>

#include "config.h"
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

static void load_players(struct server *old, struct server *new, struct player *players)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	char *pname;

	const char query[] =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ?";

	for (i = 0; i < new->num_clients; i++) {
		pname = new->clients[i].name;

		/* Don't load duplicated players */
		if (is_already_loaded(players, pname))
			continue;

		foreach_player(query, players, "s", pname);
		if (!res)
			continue;
		if (!nrow)
			create_player(players, pname);

		players->new = &new->clients[i];
		players->old = find_client(old, pname);

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

static void print_elo_change(struct player *player, int elo)
{
	verbose(
		"\t%-16s | %d -> %d (%+d)\n",
		player->name, player->elo, elo, elo - player->elo);
}

static void update_elos(struct player *players)
{
	int elos[MAX_CLIENTS];
	struct player *p;
	unsigned i;

	assert(players != NULL);

	/*
	 * Do it in two steps so that newly computed elos values do not
	 * interfer with the computing of the next ones.
	 */

	_foreach_player(p) {
		if (p->is_rankable) {
			elos[i] = compute_new_elo(p, players);
			print_elo_change(p, elos[i]);
		}
	}

	_foreach_player(p) {
		if (p->is_rankable)
			p->elo = elos[i];
	}
}

static void read_rank(sqlite3_stmt *res, void *_rank)
{
	unsigned *rank = _rank;
	*rank = sqlite3_column_int64(res, 0);
}

static void record_rank(struct player *players)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	struct player *p;

	const char query[] =
		"SELECT" RANK_COLUMN
		" FROM players"
		" WHERE name = ?";

	_foreach_player(p) {
		if (p->is_rankable) {
			foreach_row(query, read_rank, &p->rank, "s", p->name);
			if (res && nrow)
				record_elo_and_rank(p);
		}
	}
}

void rank_players(struct server *old, struct server *new)
{
	struct player players[MAX_CLIENTS] = { 0 }, *p;
	unsigned i;

	load_players(old, new, players);
	mark_rankable_players(old, new, players);
	update_elos(players);

	_foreach_player(p) {
		set_lastseen(p, new->ip, new->port);
		set_clan(p, p->new->clan);
		write_player(p);
	}

	record_rank(players);
}
