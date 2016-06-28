#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "player.h"

static int ignored;

static void ignore_newest_player(void)
{
	ignored = 1;
}

static struct player_summary *new_player(
	struct player_summary **_players, unsigned *_nplayers)
{
	static const unsigned STEP = 1024 * 1024;
	struct player_summary *players = *_players;
	unsigned nplayers = *_nplayers;

	if (ignored) {
		ignored = 0;
		return &players[nplayers - 1];
	}

	if (nplayers % STEP == 0) {
		struct player_summary *tmp;

		tmp = realloc(players, (nplayers + STEP) * sizeof(*players));
		if (!tmp) {
			fprintf(stderr, "realloc(%u): %s\n",
			        nplayers, strerror(errno));
			exit(EXIT_FAILURE);
		}
		players = tmp;
	}

	*_players = players;
	*_nplayers = nplayers + 1;

	return &players[nplayers];
}

static struct player_summary *load_all_players(unsigned *nplayers)
{
	DIR *dir;
	struct dirent *dp;
	static char path[PATH_MAX];

	struct player_summary *players = NULL;

	assert(nplayers != NULL);

	*nplayers = 0;

	sprintf(path, "%s/players", config.root);
	if (!(dir = opendir(path)))
		perror(path), exit(EXIT_FAILURE);

	while ((dp = readdir(dir))) {
		struct player_summary *player;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (!is_valid_hexname(dp->d_name))
			continue;

		player = new_player(&players, nplayers);
		if (!read_player_summary(player, dp->d_name)) {
			ignore_newest_player();
			continue;
		}
	}

	closedir(dir);

	return players;
}

static int cmp_players_elo(const void *p1, const void *p2)
{
	const struct player_summary *a = p1, *b = p2;

	/* We want them in reverse order */
	return b->elo - a->elo;
}

#define _str(s) #s
#define str(s) _str(s)

#define TEST (HEXNAME_LENGTH - 1)

static void write_ranks(struct player_summary *players, unsigned nplayers)
{
	unsigned i;
	FILE *file;
	char path[PATH_MAX];

	struct player player;

	sprintf(path, "%s/ranks", config.root);
	if (!(file = fopen(path, "w"))) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	/* Print the number of players first... */
	fprintf(file, "%u players\n", nplayers);

	/* ...and print player's name, and then save player infos themself. */
	init_player(&player);
	for (i = 0; i < nplayers; i++) {
		/*
		 * Name is padded so each line is exactly
		 * HEXNAME_LENGTH bytes.  This is done so that
		 * seeking by a n * HEXNAME_LENGTH bytes does
		 * end up on the nth name.
		 */
		fprintf(file, "%-*s\n", HEXNAME_LENGTH - 1, players[i].name);

		if (!read_player(&player, players[i].name))
			continue;

		set_rank(&player, i + 1);
		write_player(&player);
	}

	fclose(file);
}

int main(int argc, char *argv[])
{
	unsigned nplayers;
	struct player_summary *players;

	load_config();
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	players = load_all_players(&nplayers);
	qsort(players, nplayers, sizeof(*players), cmp_players_elo);

	write_ranks(players, nplayers);

	return EXIT_SUCCESS;
}
