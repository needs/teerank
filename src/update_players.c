#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#include "delta.h"
#include "elo.h"
#include "io.h"

static char *join(char *dir1, char *dir2, char *filename)
{
	static char path[PATH_MAX];
	sprintf(path, "%s/%s/%s", dir1, dir2, filename);
	return path;
}

static char *dir;

enum status {
	ERROR = 0,
	NEW, EXIST
};

static enum status create_directory(struct player_delta *player)
{
	char *path;

	assert(player != NULL);

	path = join(dir, player->name, "");

	if (mkdir(path, S_IRWXU) == -1) {
		if (errno == EEXIST)
			return EXIST;
		perror(path);
		return ERROR;
	}

	return NEW;
}

static void remove_unrankable_players(struct delta *delta)
{
	unsigned i;

	assert(delta != NULL);

	for (i = 0; i < delta->length; i++) {
		struct player_delta *player = &delta->players[i];

		/*
		 * A positive delta is necessary to not punish player's AFK
		 * and also to avoid the case were someone play on a server
		 * full of AFK or newbies to farm his rank.
		 */
		if (player->score <= 0 || player->delta <= 0) {
			delta->players[i] = delta->players[delta->length - 1];
			delta->length--;
			i--;
		}
	}
}

static unsigned is_rankable(struct delta *delta)
{
	assert(delta != NULL);

	/* At least 2 minute should have passed since the last update,
	 * otherwise the score delta is not meaningful enough. */
	if (delta->elapsed < 3 * 60)
		return 0;

	/* We need at least 4 players really playing the game (ie. rankable)
	 * to get meaningful ELO deltas. */
	remove_unrankable_players(delta);
	if (delta->length < 4)
		return 0;

	return 1;
}

static int load_elo(struct player_delta *player, int force_init)
{
	char *path;

	assert(player != NULL);

	path = join(dir, player->name, "elo");

	if (force_init)
		goto init;
	if (!read_file(path, "%d", &player->elo)) {
		if (errno != ENOENT)
			return 0;
	init:
		/*
		 * Create "elo" file so the player will still appear
		 * in database even if he is unrankable.
		 */
		write_file(path, "%d", 1500);
		player->elo = 1500;
	}

	return 1;
}

int main(int argc, char **argv)
{
	struct delta delta;
	unsigned i;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	dir = argv[1];

next:
	while (scan_delta(&delta)) {
		/*
		 * Create player if needed, Load player's ELO and update
		 * their clan.
		 */
		for (i = 0; i < delta.length; i++) {
			struct player_delta *player = &delta.players[i];
			enum status status = create_directory(player);

			if (status == ERROR)
				goto next;
			if (!load_elo(player, status == NEW))
				goto next;

			write_file(join(dir, player->name, "clan"),
			           "%s", player->clan);
		}

		/*
		 * Now that every players in delta have their ELO score loaded
		 * we can compute for each players his new ELO and write it if
		 * it has changed.
		 */

		if (!is_rankable(&delta))
			goto next;

		for (i = 0; i < delta.length; i++) {
			struct player_delta *player = &delta.players[i];
			int elo;
			char name[MAX_NAME_LENGTH];

			elo = compute_new_elo(&delta, player);

			hex_to_string(player->name, name);
			printf("%s: %d -> %d\n", name, player->elo, elo);
			if (elo != player->elo)
				write_file(join(dir, player->name, "elo"),
				           "%d", elo);
		}
	}

	return EXIT_SUCCESS;
}
