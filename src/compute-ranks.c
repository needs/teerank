#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#include "io.h"
#include "config.h"

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

		/* Name */
		strcpy(player.name, dp->d_name);

		/* Elo */
		sprintf(path, "%s/players/%s/elo", config.root, dp->d_name);
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

	/* ...and then dump player's name, one per line. */
	for (i = 0; i < array.length; i++) {
		struct player *player = &array.players[i];
		static char path[PATH_MAX];

		sprintf(path, "%s/players/%s/rank", config.root, player->name);
		if (write_file(path, "%u", i + 1) == -1)
			perror(path);

		fprintf(file, "%s\n", player->name);
	}

	fclose(file);

	return EXIT_SUCCESS;
}
