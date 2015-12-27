#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include "io.h"
#include "config.h"

struct player {
	char name[MAX_NAME_LENGTH];
	char clan[MAX_NAME_LENGTH], clan_hex[MAX_NAME_LENGTH];
	int elo;
	unsigned rank;
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

static const struct player_array PLAYER_ARRAY_ZERO;

int main(int argc, char *argv[])
{
	struct player_array array = PLAYER_ARRAY_ZERO;
	unsigned i;

	load_config();
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	load_players(argv[1], &array);
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	print_header(CTF_TAB);
	printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");

	for (i = 0; i < array.length; i++) {
		struct player *player = &array.players[i];

		printf("<tr>");

		/* Rank */
		if (player->rank == UNKNOWN_RANK)
			printf("<td>?</td>");
		else
			printf("<td>%u</td>", player->rank);

		/* Name */
		printf("<td>");
		html(player->name);
		printf("</td>");

		/* Clan */
		printf("<td>");
		if (player->clan[0] != '\0') {
			printf("<a href=\"clans/%s.html\">", player->clan_hex);
			html(player->clan);
			printf("</a>");
		}
		printf("</td>");

		/* Elo */
		if (player->elo == UNKNOWN_ELO)
			printf("<td>?</td>");
		else
			printf("<td>%d</td>", player->elo);

		printf("</tr>\n");
	}

	printf("</tbody></table>\n");
	print_footer();

	return EXIT_SUCCESS;
}
