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
		char tmp[HEXNAME_LENGTH];
		/* Put the old clan in the delta so we can use it later */
		strcpy(tmp, player->clan);
		set_clan(player, delta->clan);
		strcpy(delta->clan, tmp);
	}

	player->is_rankable = 0;
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

	for (i = 0; i < MAX_PLAYERS; i++)
		init_player(&players[i]);

	while (scan_delta(&delta)) {
		unsigned length = 0;

		/* Load player (ignore fail) */
		for (i = 0; i < delta.length; i++) {
			struct player *player;
			const char *name;
			int ret;

			player = &players[length];
			name = delta.players[i].name;

			ret = read_player(player, name);

			if (ret == FAILURE)
				continue;
			else if (ret == NOT_FOUND)
				create_player(player, name);

			merge_delta(player, &delta.players[i]);
			length++;
		}

		/* Compute their new elos */
		if (mark_rankable_players(&delta, players, length))
			update_elos(players, length);

		/* Update lastseen and write players */
		for (i = 0; i < length; i++) {
			set_lastseen(&players[i], delta.ip, delta.port);

			if (!write_player(&players[i]))
				continue;

			if (players[i].clan_changed) {
				printf("%s %s %s\n",
				       players[i].name,
				       players[i].delta->clan,
				       players[i].clan);
			}
		}
	}

	return EXIT_SUCCESS;
}
