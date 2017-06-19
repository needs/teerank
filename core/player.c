#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "player.h"
#include "teerank.h"

void create_player(const char *name, const char *clan)
{
	const char *query =
		"INSERT INTO players VALUES(?, ?, ?, NULL, NULL)";

	assert(name != NULL);
	assert(clan != NULL);

	exec(query, "sst", name, clan, time(NULL));
}

void read_player(sqlite3_stmt *res, void *_p)
{
	struct player *p = _p;

	snprintf(p->name, sizeof(p->name), "%s", sqlite3_column_text(res, 0));
	snprintf(p->clan, sizeof(p->clan), "%s", sqlite3_column_text(res, 1));
	p->lastseen = sqlite3_column_int64(res, 2);
	snprintf(p->server_ip, sizeof(p->server_ip), "%s", sqlite3_column_text(res, 3));
	snprintf(p->server_port, sizeof(p->server_port), "%s", sqlite3_column_text(res, 4));

	snprintf(p->gametype, sizeof(p->gametype), "%s", sqlite3_column_text(res, 5));
	snprintf(p->map, sizeof(p->map), "%s", sqlite3_column_text(res, 6));
	p->elo = sqlite3_column_int(res, 7);
	p->rank = sqlite3_column_int64(res, 8);
}

void read_player_record(sqlite3_stmt *res, void *_r)
{
	struct player_record *r = _r;

	r->ts = sqlite3_column_int64(res, 0);
	r->elo = sqlite3_column_int(res, 1);
	r->rank = sqlite3_column_int64(res, 2);
}

unsigned count_ranked_players(const char *gametype, const char *map)
{
	const char *query =
		"SELECT COUNT(1)"
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE gametype = ? AND map = ?";

	return count_rows(query, "ss", gametype, map);
}
