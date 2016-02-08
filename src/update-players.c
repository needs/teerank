#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#include "delta.h"
#include "elo.h"
#include "io.h"
#include "config.h"

enum status {
	ERROR = 0,
	NEW, EXIST
};

static enum status create_directory(struct player_delta *player)
{
	static char path[PATH_MAX];

	assert(player != NULL);

	sprintf(path, "%s/players/%s", config.root, player->name);
	if (mkdir(path, 0777) == -1) {
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
	unsigned old_length;

	assert(delta != NULL);

	/*
	 * 30 minutes between each update is just too much and it increase
	 * the chance of rating two different games.
	 */
	if (delta->elapsed > 30 * 60) {
		verbose("A game with %u players is unrankable because too"
		        " much time have passed between two updates\n",
		        delta->length);
		return 0;
	}

	/*
	 * We need at least 4 players really playing the game (ie. rankable)
	 * to get meaningful Elo deltas.
	 */
	old_length = delta->length;
	remove_unrankable_players(delta);
	if (delta->length < 4) {
		verbose("A game with %u players is unrankable because only"
		        " %u players can be ranked, 4 needed\n",
		        old_length, old_length - delta->length);
		return 0;
	}

	return 1;
}

static int load_elo(struct player_delta *player, int force_init)
{
	static char path[PATH_MAX];
	int ret;

	assert(player != NULL);

	sprintf(path, "%s/players/%s/elo", config.root, player->name);
	if (force_init)
		goto init;

	ret = read_file(path, "%d", &player->elo);
	if (ret == 1)
		return 1;
	if (ret == 0)
		return fprintf(stderr, "%s: Cannot scan for Elo points\n", path), 0;
	if (ret == -1 && errno != ENOENT)
		return perror(path), 0;

init:
	/*
	 * Create "elo" file so the player will still appear
	 * in database even if he is unrankable.
	 *
	 * If the write fail it's no big deal because if the player is:
	 *   - rankable: another write_file() will be attempted to
	 *     save his new Elo.
	 *   - unrankable: We will try again next time.
	 */
	write_file(path, "%d", 1500);
	player->elo = 1500;

	return 1;
}

int main(int argc, char **argv)
{
	struct delta delta;
	static char path[PATH_MAX];
	unsigned i;

	load_config();
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

next:
	while (scan_delta(&delta)) {
		/*
		 * Create player if needed, Load player's Elo and update
		 * their clan.
		 */
		for (i = 0; i < delta.length; i++) {
			struct player_delta *player = &delta.players[i];
			enum status status = create_directory(player);

			if (status == ERROR)
				goto next;
			if (!load_elo(player, status == NEW))
				goto next;

			sprintf(path, "%s/players/%s/clan",
			        config.root, player->name);
			write_file(path, "%s", player->clan);
		}

		/*
		 * Now that every players in delta have their Elo score loaded,
		 * we can compute for each players his new Elo and write it if
		 * it has changed.
		 */

		if (!is_rankable(&delta))
			goto next;

		verbose("%u players have been updated:\n", delta.length);
		for (i = 0; i < delta.length; i++) {
			struct player_delta *player = &delta.players[i];
			int elo;
			char name[MAX_NAME_LENGTH];

			elo = compute_new_elo(&delta, player);
			hex_to_string(player->name, name);

			verbose("\t%s \t%d -> %d\n", name, player->elo, elo);
			if (elo == player->elo)
				continue;
			sprintf(path, "%s/players/%s/elo",
			        config.root, player->name);
			if (write_file(path, "%d", elo) == -1)
				perror(path);
		}
	}

	return EXIT_SUCCESS;
}
