#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#include "io.h"
#include "config.h"
#include "player.h"

static void load_players(struct player_array *array)
{
	DIR *dir;
	struct dirent *dp;
	static char path[PATH_MAX];

	assert(array != NULL);

	sprintf(path, "%s/players", config.root);
	if (!(dir = opendir(path)))
		perror(path), exit(EXIT_FAILURE);

	while ((dp = readdir(dir))) {
		struct player player;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		if (read_player(&player, dp->d_name))
			add_player(array, &player);
	}

	closedir(dir);
}

static int cmp_player(const void *p1, const void *p2)
{
	const struct player *a = p1, *b = p2;

	/* We want them in reverse order */
	return b->elo - a->elo;
}

static const struct player_array PLAYER_ARRAY_ZERO;

int main(int argc, char *argv[])
{
	struct player_array array = PLAYER_ARRAY_ZERO;
	unsigned i;
	FILE *file;
	char path[PATH_MAX];

	load_config();
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	load_players(&array);
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	sprintf(path, "%s/ranks", config.root);
	if (!(file = fopen(path, "w")))
		return perror(path), EXIT_FAILURE;

	/* Print the number of players first... */
	fprintf(file, "%u players\n", array.length);

	/* ...and print player's name, and then save player infos themself. */
	for (i = 0; i < array.length; i++) {
		struct player *player = &array.players[i];

		fprintf(file, "%s\n", player->name);
		player->rank = i + 1;
		write_player(player);
	}

	fclose(file);

	return EXIT_SUCCESS;
}
