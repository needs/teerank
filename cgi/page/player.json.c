#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "player.h"
#include "json.h"

static void json_player(struct player *player)
{
	printf("\"name\":\"%s\",", json_hexstring(player->name));
	printf("\"clan\":\"%s\",", json_hexstring(player->clan));
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

	const char *query =
		"SELECT" ALL_PLAYER_RECORD_COLUMNS
		" FROM ranks_historic"
		" WHERE name = ?"
		" ORDER BY ts";

	printf(",\"historic\":{\"records\":[");

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

int main_json_player(struct url *url)
{
	struct player player;
	char *pname;
	int full = 0;
	unsigned i;

	sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE players.name = ? AND gametype = '' AND map = ''";

	for (i = 0; i < url->nargs; i++)
		if (strcmp(url->args[i].name, "short") == 0)
			full = 0;

	pname = url->dirs[1];

	foreach_player(query, &player, "s", pname);
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
