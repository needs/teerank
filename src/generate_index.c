#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include "io.h"

static void print_file(char *path)
{
	FILE *file = NULL;
	int c;

	if (!(file = fopen(path, "r")))
		exit(EXIT_FAILURE);

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
}

struct player {
	char name[MAX_NAME_LENGTH];
	char clan[MAX_NAME_LENGTH], clan_hex[MAX_NAME_LENGTH];
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
		hex_to_string(dp->d_name, player.name);

		/* Clan */
		sprintf(path, "%s/%s/%s", root, dp->d_name, "clan");
		if (read_file(path, "%s", player.clan_hex) == 1)
			hex_to_string(player.clan_hex, player.clan);
		else
			player.clan[0] = '\0';

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
		fprintf(stderr, "Usage: %s <player_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	load_players(argv[1], &array);
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	print_file("html/header.inc.html");
	for (i = 0; i < array.length; i++) {
		struct player *player = &array.players[i];

		printf("<tr><td>%u</td><td>", i + 1);
		html(player->name);
		printf("</td><td>");

		if (player->clan != '\0') {
			printf("<a href=\"clans/%s.html\">", player->clan_hex);
			html(player->clan);
			printf("</a>");
		}
		printf("</td><td>%d</td></tr>\n", player->elo);
	}
	print_file("html/footer.inc.html");

	return EXIT_SUCCESS;
}
