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
#include "json.h"

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

static void json_player_historic(const char *pname)
{
	struct json_list_column cols[] = {
		{ "timestamp", "%d" },
		{ "elo", "%i" },
		{ "rank", "%u" },
		{ NULL }
	};

	const char *query =
		"SELECT" ALL_PLAYER_RECORD_COLUMNS
		" FROM ranks_historic"
		" WHERE name IS ?"
		" ORDER BY ts";

	json(",%s:{%s:", "historic", "records");
	json_array_list(foreach_init(query, "s", pname), cols, "length");
	json("}");
}

void generate_json_player(struct url *url)
{
	struct player player;
	char *pname = NULL;
	bool full = true;
	unsigned i;

	sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT players.name, clan, elo, rank, lastseen, server_ip, server_port"
		" FROM players LEFT OUTER JOIN ranks ON players.name = ranks.name"
		" WHERE players.name IS ?";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			pname = url->args[i].val;
		else if (strcmp(url->args[i].name, "short") == 0)
			full = false;
	}

	foreach_player(query, &player, "s", pname);
	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);

	json("{");

	json_player(&player);
	if (full)
		json_player_historic(player.name);

	json("}");
}
