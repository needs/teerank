#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "player.h"
#include "server.h"
#include "delta.h"
#include "elo.h"

/*
 * Given a game it does return how many players are rankable.
 */
static unsigned mark_rankable_players(
	struct delta *delta, struct player *players, unsigned length)
{
	unsigned i, rankable = 0;

	assert(players != NULL);

	/*
	 * New server will have an elapsed time equals to zero.  Make it
	 * a special case so we don't print a verbose message, since
	 * add-new-servers already printed one.
	 */
	if (delta->elapsed == 0)
		return 0;

	/*
	 * 30 minutes between each update is just too much and it increase
	 * the chance of rating two different games.
	 */
	if (delta->elapsed > 30 * 60) {
		verbose("A game with %u players is unrankable because too"
		        " much time have passed between two updates\n",
		        length);
		return 0;
	}

	/*
	 * On the other hand, less than 1 minutes between updates is
	 * also meaningless.
	 */
	if (delta->elapsed < 60) {
		verbose("A game with %u players is unrankable because too"
		        " little time have passed between two updates\n",
		        length);
		return 0;
	}

	if (!is_vanilla_ctf_server(
		    delta->gametype, delta->map,
		    delta->num_clients, delta->max_clients))
		return 0;


	/* Mark rankable players */
	for (i = 0; i < length; i++) {
		if (players[i].delta->ingame) {
			players[i].is_rankable = 1;
			rankable++;
		}
	}

	/*
	 * We don't rank games with less than 4 rankable players.  We believe
	 * it is too much volatile to rank those kind of games.
	 */
	if (rankable < 4) {
		verbose("A game with %u players is unrankable because only"
		        " %u players can be ranked, 4 needed\n",
		        length, rankable);
		return 0;
	}

	verbose("A game with %u rankable players over %u will be ranked\n",
	        rankable, length);

	return rankable;
}

static void merge_delta(struct player *player, struct player_delta *delta)
{
	assert(player != NULL);
	assert(delta != NULL);

	player->delta = delta;

	/*
	 * Store the old clan to be able to write a delta once
	 * write_player() succeed.
	 */
	if (strcmp(player->clan, delta->clan) != 0) {
		char tmp[NAME_LENGTH];
		/* Put the old clan in the delta so we can use it later */
		strcpy(tmp, player->clan);
		set_clan(player, delta->clan);
		strcpy(delta->clan, tmp);
	}

	player->is_rankable = 0;
}

static void read_rank(sqlite3_stmt *res, void *_rank)
{
	unsigned *rank = _rank;
	*rank = sqlite3_column_int64(res, 0);
}

static void compute_ranks(struct player *players, unsigned len)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	struct player *p;

	const char query[] =
		"SELECT" RANK_COLUMN
		" FROM players"
		" WHERE name = ?";

	for (i = 0; i < len; i++) {
		p = &players[i];

		if (!p->is_rankable)
			continue;

		foreach_row(query, read_rank, &p->rank, "s", p->name);
		record_elo_and_rank(p);
	}
}

static int load_player(struct player *p, const char *pname)
{
	unsigned nrow;
	sqlite3_stmt *res;

	const char query[] =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ?";

	foreach_player(query, p, "s", pname);

	if (!res)
		return 0;
	if (!nrow)
		create_player(p, pname);

	return 1;
}

int main(int argc, char **argv)
{
	struct delta delta;
	struct player players[MAX_PLAYERS];
	unsigned i;

	load_config(1);
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	while (scan_delta(&delta)) {
		unsigned length = 0;
		int rankedgame;

		exec("BEGIN");

		/* Load player (ignore fail) */
		for (i = 0; i < delta.length; i++) {
			struct player *player;
			const char *name;

			player = &players[length];
			name = delta.players[i].name;

			if (load_player(player, name)) {
				merge_delta(player, &delta.players[i]);
				length++;
			}
		}

		rankedgame = mark_rankable_players(&delta, players, length);

		if (rankedgame)
			compute_elos(players, length);

		for (i = 0; i < length; i++) {
			set_lastseen(&players[i], delta.ip, delta.port);
			write_player(&players[i]);
		}

		/*
		 * We need player to have been written to the database
		 * before calculating rank, because such calculation
		 * only use what's in the database.
		 */
		if (rankedgame)
			compute_ranks(players, length);

		exec("COMMIT");
	}

	return EXIT_SUCCESS;
}
