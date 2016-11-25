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
#include "config.h"
#include "elo.h"

void init_player(struct player *player)
{
	static const struct player PLAYER_ZERO;
	*player = PLAYER_ZERO;
}

void create_player(struct player *player, const char *name)
{
	assert(player != NULL);

	strcpy(player->name, name);
	strcpy(player->clan, "00");

	player->elo = DEFAULT_ELO;
	player->rank = UNRANKED;

	player->lastseen = time(NULL);
	strcpy(player->server_ip, "");
	strcpy(player->server_port, "");

	player->is_rankable = 0;
}

static void _read_player(sqlite3_stmt *res, struct player *p, int extended)
{
	snprintf(p->name, sizeof(p->name), "%s", sqlite3_column_text(res, 0));
	snprintf(p->clan, sizeof(p->clan), "%s", sqlite3_column_text(res, 1));
	p->elo = sqlite3_column_int(res, 2);
	p->lastseen = sqlite3_column_int64(res, 3);
	snprintf(p->server_ip, sizeof(p->server_ip), "%s", sqlite3_column_text(res, 4));
	snprintf(p->server_port, sizeof(p->server_port), "%s", sqlite3_column_text(res, 5));

	if (extended) {
		p->rank = sqlite3_column_int64(res, 6);
	} else {
		p->rank = UNRANKED;
	}
}

void read_player(sqlite3_stmt *res, void *p)
{
	_read_player(res, p, 0);
}

void read_extended_player(sqlite3_stmt *res, void *p)
{
	_read_player(res, p, 1);
}

void read_player_record(sqlite3_stmt *res, void *_r)
{
	struct player_record *r = _r;

	r->ts = sqlite3_column_int64(res, 0);
	r->elo = sqlite3_column_int(res, 1);
	r->rank = sqlite3_column_int64(res, 2);
}

int write_player(struct player *p)
{
	const char query[] =
		"INSERT OR REPLACE INTO players"
		" VALUES (?, ?, ?, ?, ?, ?)";

	if (exec(query, "ssitss", p->name, p->clan, p->elo, p->lastseen, p->server_ip, p->server_port))
		return SUCCESS;
	else
		return FAILURE;
}

void set_clan(struct player *player, char *clan)
{
	assert(player != NULL);
	assert(clan != NULL);

	strcpy(player->clan, clan);
	player->clan_changed = 1;
}

void set_lastseen(struct player *player, const char *ip, const char *port)
{
	assert(player != NULL);

	player->lastseen = time(NULL);
	strcpy(player->server_ip, ip);
	strcpy(player->server_port, port);
}

void record_elo_and_rank(struct player *p)
{
	const char query[] =
		"INSERT OR REPLACE INTO player_historic"
		" VALUES (?, ?, ?, ?)";

	assert(p != NULL);
	assert(p->elo != INVALID_ELO);
	assert(p->rank != UNRANKED);

	exec(query, "stiu", p->name, time(NULL), p->elo, p->rank);
}

unsigned count_players(void)
{
	char query[] =
		"SELECT COUNT(1) FROM players";

	return count_rows(query);
}
