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

	if (array->length % OFFSET == 0) {
		struct player *players;
		players = realloc(array->players, (array->length + OFFSET) * sizeof(*player));
		if (!players)
			return perror("Re-allocating player array");
		array->players = players;
	}

	array->players[array->length++] = *player;
}

static const int UNKNOWN_ELO = INT_MIN;
static const unsigned UNKNOWN_RANK = UINT_MAX;

static char *root;

static void load_player(struct player *player, char *name)
{
	static char path[PATH_MAX];

	assert(player != NULL);

	/* Name */
	hex_to_string(name, player->name);

	/* Clan */
	sprintf(path, "%s/%s/%s", root, name, "clan");
	if (read_file(path, "%s", player->clan_hex) == 1)
		hex_to_string(player->clan_hex, player->clan);
	else
		player->clan[0] = '\0';

	/* Elo */
	sprintf(path, "%s/%s/%s", root, name, "elo");
	if (read_file(path, "%d", &player->elo) != 1)
		player->elo = UNKNOWN_ELO;

	/* Rank */
	sprintf(path, "%s/%s/%s", root, name, "rank");
	if (read_file(path, "%u", &player->rank) != 1)
		player->rank = UNKNOWN_RANK;
}

static void load_players(struct player_array *array)
{
	char name[MAX_NAME_LENGTH];

	assert(array != NULL);

	while (scanf("%s", name) == 1) {
		struct player player;
		load_player(&player, name);
		add_player(array, &player);
	}
}

static void print_array(struct player_array *array)
{
	unsigned i;

	assert(array != NULL);

	for (i = 0; i < array->length; i++) {
		struct player *player = &array->players[i];

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
			printf("<a href=\"/clans/%s.html\">", player->clan_hex);
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
}

static const struct player_array PLAYER_ARRAY_ZERO;
enum mode {
	FULL_PAGE, ONLY_ROWS
};

int main(int argc, char **argv)
{
	struct player_array array = PLAYER_ARRAY_ZERO;
	enum mode mode;

	if (argc != 3) {
		fprintf(stderr, "usage: %s [full-page|only-rows] <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	root = argv[2];
	if (!strcmp(argv[1], "full-page"))
		mode = FULL_PAGE;
	else if (!strcmp(argv[1], "only-rows"))
		mode = ONLY_ROWS;
	else {
		fprintf(stderr, "First argument must be either \"full-page\" or \"only-rows\"\n");
		return EXIT_FAILURE;
	}

	load_players(&array);

	if (mode == FULL_PAGE) {
		print_header();
		printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");
	}
	print_array(&array);
	if (mode == FULL_PAGE) {
		printf("</tbody></table>");
		print_footer();
	}

	return EXIT_SUCCESS;
}
