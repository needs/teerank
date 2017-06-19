#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "json.h"

static void json_player(sqlite3_stmt *res, void *unused)
{
	json("%s:%s,", "name", (char *)sqlite3_column_text(res, 0));
	json("%s:%s,", "clan", (char *)sqlite3_column_text(res, 1));
	json("%s:%i,", "elo", sqlite3_column_int(res, 2));
	json("%s:%u,", "rank", (unsigned)sqlite3_column_int64(res, 3));
	json("%s:%d,", "lastseen", (time_t)sqlite3_column_int64(res, 4));
	json("%s:%s,", "server_ip", (char *)sqlite3_column_text(res, 5));
	json("%s:%s", "server_port", (char *)sqlite3_column_text(res, 6));
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
		"SELECT ts, elo, rank"
		" FROM ranks_historic"
		" WHERE name IS ?"
		" ORDER BY ts";

	json(",%s:{%s:", "historic", "records");
	json_array_list(foreach_init(query, "s", pname), cols, "length");
	json("}");
}

void generate_json_player(struct url *url)
{
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

	json("{");

	foreach_row(query, json_player, NULL, "s", pname);

	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);
	if (full)
		json_player_historic(pname);

	json("}");
}
