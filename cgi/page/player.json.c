#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "json.h"

static void json_player(struct player *player)
{
	printf("\"name\":\"%s\",", json_escape(player->name));
	printf("\"clan\":\"%s\",", json_escape(player->clan));
	printf("\"elo\":%d,", player->elo);
	printf("\"rank\":%u,", player->rank);
	printf("\"lastseen\":\"%s\",", json_date(player->lastseen));
	printf("\"server_ip\":\"%s\",", player->server_ip);
	printf("\"server_port\":\"%s\"", player->server_port);
}

static int json_player_historic(const char *pname)
{
	time_t epoch = 0;
	unsigned nrow;
	sqlite3_stmt *res;
	struct player_record r;

	char query[] =
		"SELECT" ALL_PLAYER_RECORD_COLUMNS
		" FROM player_historic"
		" WHERE name = ?"
		" ORDER BY timestamp";

	printf("\"historic\":{\"records\":[");

	foreach_player_record(query, &r, "s", pname) {
		if (!nrow)
			epoch = r.ts;
		else
			putchar(',');

		printf("[%ju, %d, %u]", (uintmax_t)(r.ts - epoch), r.elo, r.rank);
	}

	printf("],\"epoch\":%ju,\"length\":%u}", (uintmax_t)epoch, nrow);

	return res != NULL;
}

int main_json_player(int argc, char **argv)
{
	struct player player;
	int full;

	sqlite3_stmt *res;
	unsigned nrow;

	const char query[] =
		"SELECT" ALL_EXTENDED_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ?";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <player_name> full|short\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[2], "full") == 0)
		full = 1;
	else if (strcmp(argv[2], "short") == 0)
		full = 0;
	else {
		fprintf(stderr, "%s: Should be either \"full\" or \"short\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	foreach_extended_player(query, &player, "s", argv[1]);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	putchar('{');

	json_player(&player);
	if (full && !json_player_historic(player.name))
		return EXIT_FAILURE;

	putchar('}');

	return EXIT_SUCCESS;
}
