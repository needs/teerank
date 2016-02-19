#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#include "io.h"
#include "config.h"
#include "player.h"

struct player_array {
	struct player *players;
	unsigned length, buffer_length;
};

static struct player *new_player(struct player_array *array)
{
	static const unsigned OFFSET = 1024;

	assert(array != NULL);

	if (array->length == array->buffer_length) {
		array->players = realloc(array->players, (array->buffer_length + OFFSET) * sizeof(*array->players));
		if (!array->players)
			perror("Allocating player array"), exit(EXIT_FAILURE);
		array->buffer_length += OFFSET;
	}

	return &array->players[array->length++];
}

static void delete_last_player(struct player_array *array)
{
	assert(array != NULL);
	assert(array->length > 0);

	array->length--;
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
		struct player *player;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		player = new_player(array);
		if (!read_player(player, dp->d_name))
			delete_last_player(array);
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
