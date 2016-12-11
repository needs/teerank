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

void init_player(struct player *player)
{
	static const struct player PLAYER_ZERO;
	*player = PLAYER_ZERO;
}

void create_player(const char *name, const char *clan)
{
	struct player p;

	assert(name != NULL);
	assert(clan != NULL);

	snprintf(p.name, sizeof(p.name), "%s", name);
	snprintf(p.clan, sizeof(p.clan), "%s", clan);

	p.elo = DEFAULT_ELO;
	p.rank = UNRANKED;

	p.lastseen = time(NULL);
	strcpy(p.server_ip, "");
	strcpy(p.server_port, "");

	write_player(&p);
}

void read_player(sqlite3_stmt *res, void *_p)
{
	struct player *p = _p;

	snprintf(p->name, sizeof(p->name), "%s", sqlite3_column_text(res, 0));
	snprintf(p->clan, sizeof(p->clan), "%s", sqlite3_column_text(res, 1));
	p->elo = sqlite3_column_int(res, 2);
	p->rank = sqlite3_column_int64(res, 3);
	p->lastseen = sqlite3_column_int64(res, 4);
	snprintf(p->server_ip, sizeof(p->server_ip), "%s", sqlite3_column_text(res, 5));
	snprintf(p->server_port, sizeof(p->server_port), "%s", sqlite3_column_text(res, 6));
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
	const char *query =
		"INSERT OR REPLACE INTO players"
		" VALUES (?, ?, ?, ?, ?, ?, ?)";

	if (exec(query, "ssiutss", p->name, p->clan, p->elo, p->rank, p->lastseen, p->server_ip, p->server_port))
		return SUCCESS;
	else
		return FAILURE;
}

void record_elo_and_rank(struct player *p)
{
	const char *query =
		"INSERT OR REPLACE INTO player_historic"
		" VALUES (?, ?, ?, ?)";

	assert(p != NULL);
	assert(p->rank != UNRANKED);

	exec(query, "stiu", p->name, time(NULL), p->elo, p->rank);
}

unsigned count_ranked_players(void)
{
	const char *query =
		"SELECT COUNT(1) FROM players WHERE" IS_PLAYER_RANKED;

	return count_rows(query);
}
