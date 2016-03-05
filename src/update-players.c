#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "delta.h"
#include "elo.h"
#include "io.h"
#include "config.h"

static unsigned make_sens_to_rank(
	int elapsed, struct player *players, unsigned length)
{
	unsigned i, rankable = 0;

	assert(players != NULL);

	/*
	 * 30 minutes between each update is just too much and it increase
	 * the chance of rating two different games.
	 */
	if (elapsed > 30 * 60) {
		verbose("A game with %u players is unrankable because too"
		        " much time have passed between two updates\n",
		        length);
		return 0;
	}

	/*
	 * We don't rank games with less than 4 rankable players.  We believe
	 * it is too much volatile to rank those kind of games.
	 */
	for (i = 0; i < length; i++)
		if (players[i].is_rankable)
			rankable++;
	if (rankable < 4) {
		verbose("A game with %u players is unrankable because only"
		        " %u players can be ranked, 4 needed\n",
		        length, rankable);
		return 0;
	}

	verbose("A game with %u rankable players over %u will be ranked\n",
	        rankable, length);
	return 1;
}

static void merge_delta(struct player *player, struct player_delta *delta)
{
	assert(player != NULL);
	assert(delta != NULL);

	player->delta = delta;

	if (strcmp(player->clan, delta->clan)) {
		strcpy(player->clan, delta->clan);
		player->is_modified = 1;
	}

	player->is_rankable = 1;
}

int main(int argc, char **argv)
{
	struct delta delta;
	struct player players[MAX_PLAYERS];
	unsigned i;

	load_config();
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	while (scan_delta(&delta)) {
		unsigned length = 0;

		/* Load player (ignore fail) */
		for (i = 0; i < delta.length; i++) {
			if (!read_player(&players[length], delta.players[i].name))
				continue;

			merge_delta(&players[length], &delta.players[i]);
			length++;
		}

		/* Compute their new elos */
		if (make_sens_to_rank(delta.elapsed, players, length))
			update_elos(players, length);

		/* Write the result only if something changed */
		for (i = 0; i < length; i++)
			if (players[i].is_modified)
				write_player(&players[i]);
	}

	return EXIT_SUCCESS;
}
