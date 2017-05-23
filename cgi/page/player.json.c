#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "player.h"
#include "html.h"

static void json_player(struct player *player)
{
	json("%s:%s,", "name", player->name);
	json("%s:%s,", "clan", player->clan);
	json("%s:%i,", "elo", player->elo);
	json("%s:%u,", "rank", player->rank);
	json("%s:%d,", "lastseen", player->lastseen);
	json("%s:%s,", "server_ip", player->server_ip);
	json("%s:%s", "server_port", player->server_port);
}

static int json_player_historic(const char *pname)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct player_record r;

	const char *query =
		"SELECT" ALL_PLAYER_RECORD_COLUMNS
		" FROM ranks_historic"
		" WHERE name = ?"
		" ORDER BY ts";

	json(",%s:{%s:[", "historic", "records");

	foreach_player_record(query, &r, "s", pname) {
		if (nrow)
			json(",");

		json("[%d, %i, %u]", r.ts, r.elo, r.rank);
	}

	json("],%s:%u}", "length", nrow);

	return res != NULL;
}

int main_json_player(struct url *url)
{
	struct player player;
	char *pname = NULL;
	bool full = true;
	unsigned i;

	sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE players.name = ? AND gametype = '' AND map = ''";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			pname = url->args[i].val;
		else if (strcmp(url->args[i].name, "short") == 0)
			full = false;
	}

	foreach_player(query, &player, "s", pname);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	json("{");

	json_player(&player);
	if (full && !json_player_historic(player.name))
		return EXIT_FAILURE;

	json("}");

	return EXIT_SUCCESS;
}
