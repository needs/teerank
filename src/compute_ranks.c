#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#include "io.h"

struct player {
	char name[MAX_NAME_LENGTH];
	int elo;
};

struct player_array {
	struct player* players;
	unsigned length;
};

static void add_player(struct player_array *array, struct player *player)
{
	static const unsigned OFFSET = 1024;

	assert(array != NULL);
	assert(player != NULL);

	if (array->length % 1024 == 0) {
		array->players = realloc(array->players, (array->length + OFFSET) * sizeof(*player));
		if (!array->players)
			perror("Allocating player array"), exit(EXIT_FAILURE);
	}

	array->players[array->length++] = *player;
}

static void load_players(char *root, struct player_array *array)
{
	DIR *dir;
	struct dirent *dp;
	static char path[PATH_MAX];

	assert(root != NULL);
	assert(array != NULL);

	if (!(dir = opendir(root)))
		perror(root), exit(EXIT_FAILURE);

	while ((dp = readdir(dir))) {
		struct player player;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		/* Name */
		strcpy(player.name, dp->d_name);

		/* Elo */
		sprintf(path, "%s/%s/%s", root, dp->d_name, "elo");
		if (read_file(path, "%d", &player.elo) != 1)
			continue;

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

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	load_players(argv[1], &array);
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	for (i = 0; i < array.length; i++) {
		struct player *player = &array.players[i];
		static char path[PATH_MAX];

		sprintf(path, "%s/%s/%s", argv[1], player->name, "rank");
		if (write_file(path, "%u", i + 1) == -1)
			perror(path);

		printf("%u %s\n", i, player->name);
	}

	return EXIT_SUCCESS;
}
