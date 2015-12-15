#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>

#include "io.h"

struct player {
	char name[MAX_NAME_LENGTH];
	char clan[MAX_NAME_LENGTH], clan_hex[MAX_NAME_LENGTH];
	int elo;
	unsigned rank;
};

struct player_array {
	struct player *players;
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

static const int UNKNOWN_ELO = INT_MIN;
static const unsigned UNKNOWN_RANK = UINT_MAX;

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
			player.elo = UNKNOWN_ELO;

		/* Rank */
		sprintf(path, "%s/%s/%s", root, dp->d_name, "rank");
		if (read_file(path, "%u", &player.rank) != 1)
			player.rank = UNKNOWN_RANK;

		add_player(array, &player);
	}

	closedir(dir);
}

static int cmp_player(const void *p1, const void *p2)
{
	const struct player *a = p1, *b = p2;

	/* We want them in reverse order */
	if (b->rank > a->rank)
		return -1;
	if (b->rank < a->rank)
		return 1;
	return 0;
}

static void write_page(FILE *file, struct player_array *array,
                       unsigned start, unsigned length)
{
	unsigned i;

	assert(file != NULL);
	assert(array != NULL);
	assert(start < array->length);

	for (i = start; i < start + length && i < array->length; i++) {
		struct player *player = &array->players[i];

		fprintf(file, "<tr>");

		/* Rank */
		if (player->rank == UNKNOWN_RANK)
			fprintf(file, "<td>?</td>");
		else
			fprintf(file, "<td>%u</td>", player->rank);

		/* Name */
		fprintf(file, "<td>");
		fhtml(file, player->name);
		fprintf(file, "</td>");

		/* Clan */
		fprintf(file, "<td>");
		if (player->clan[0] != '\0') {
			fprintf(file, "<a href=\"clans/%s.html\">", player->clan_hex);
			fhtml(file, player->clan);
			fprintf(file, "</a>");
		}
		fprintf(file, "</td>");

		/* Elo */
		if (player->elo == UNKNOWN_ELO)
			fprintf(file, "<td>?</td>");
		else
			fprintf(file, "<td>%d</td>", player->elo);

		fprintf(file, "</tr>\n");
	}
}

static const struct player_array PLAYER_ARRAY_ZERO;

int main(int argc, char **argv)
{
	struct player_array array = PLAYER_ARRAY_ZERO;
	unsigned i;
	static const unsigned PLAYERS_PER_PAGE = 200;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <players_directory> <pages_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	load_players(argv[1], &array);
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	for (i = 0; i < array.length; i += PLAYERS_PER_PAGE) {
		static char path[PATH_MAX];
		FILE *file;

		sprintf(path, "%s/%u.inc.html", argv[2], i / PLAYERS_PER_PAGE);
		if (!(file = fopen(path, "w"))) {
			/*
			 * We could continue and just output a warning
			 * but as it is right now, just stop here because
			 * the user won't be able the see the next pages
			 * anyway.
			 */
			perror(path);
			return EXIT_FAILURE;
		}

		write_page(file, &array, i, PLAYERS_PER_PAGE);

		fclose(file);

		sprintf(path, "%s/%u.html", argv[2], i / PLAYERS_PER_PAGE);
		if (!(file = fopen(path, "w"))) {
			/*
			 * We could continue and just output a warning
			 * but as it is right now, just stop here because
			 * the user won't be able the see the next pages
			 * anyway.
			 */
			perror(path);
			return EXIT_FAILURE;
		}

		fprint_file(file, "html/header.inc.html");
		write_page(file, &array, i, PLAYERS_PER_PAGE);
		fprint_file(file, "html/footer.inc.html");

		fclose(file);
	}

	return EXIT_SUCCESS;
}
