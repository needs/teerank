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

static int is_valid_hexpair(const char *hex)
{
	char c1, c2;

	if (hex[0] == '\0' || hex[1] == '\0')
		return 0;

	c1 = hex[0];
	c2 = hex[1];

	if (!isxdigit(c1) || !isxdigit(c2))
		return 0;

	if (c1 == '0' && c2 == '0')
		return 0;

	return 1;
}

static int is_terminating_hexpair(const char *hex)
{
	if (hex[0] == '\0' || hex[1] == '\0')
		return 0;

	return hex[0] == '0' && hex[1] == '0' && hex[2] == '\0';
}

int is_valid_hexname(const char *hex)
{
	assert(hex != NULL);

	while (is_valid_hexpair(hex))
		hex += 2;

	return is_terminating_hexpair(hex);
}

static unsigned char hextodec(char c)
{
	switch (c) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A':
	case 'a': return 10;
	case 'B':
	case 'b': return 11;
	case 'C':
	case 'c': return 12;
	case 'D':
	case 'd': return 13;
	case 'E':
	case 'e': return 14;
	case 'F':
	case 'f': return 15;
	default: return 0;
	}
}

void hexname_to_name(const char *hex, char *name)
{
	assert(hex != NULL);
	assert(name != NULL);
	assert(hex != name);

	for (; hex[0] != '0' || hex[1] != '0'; hex += 2, name++)
		*name = hextodec(hex[0]) * 16 + hextodec(hex[1]);

	*name = '\0';
}

void name_to_hexname(const char *name, char *hex)
{
	assert(name != NULL);
	assert(hex != NULL);
	assert(name != hex);

	for (; *name; name++, hex += 2)
		sprintf(hex, "%2x", *(unsigned char*)name);
	strcpy(hex, "00");
}

void init_player(struct player *player)
{
	static const struct player PLAYER_ZERO;
	*player = PLAYER_ZERO;
}

/*
 * Set all player's fields to meaningless values suitable for printing.
 */
static void reset_player(struct player *player, const char *name)
{
	strcpy(player->name, name);
	strcpy(player->clan, "00");
	player->elo = INVALID_ELO;
	player->rank = UNRANKED;
	player->lastseen = NEVER_SEEN;

	player->clan_changed = 0;
	player->delta = NULL;
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

void player_from_result_row(struct player *player, sqlite3_stmt *res, int read_rank)
{
	snprintf(player->name, sizeof(player->name), "%s", sqlite3_column_text(res, 0));
	snprintf(player->clan, sizeof(player->clan), "%s", sqlite3_column_text(res, 1));
	player->elo = sqlite3_column_int(res, 2);
	player->lastseen = sqlite3_column_int64(res, 3);
	snprintf(player->server_ip, sizeof(player->server_ip), "%s", sqlite3_column_text(res, 4));
	snprintf(player->server_port, sizeof(player->server_port), "%s", sqlite3_column_text(res, 5));

	if (read_rank)
		player->rank = sqlite3_column_int64(res, 6);
}

int read_player(struct player *player, const char *name, int read_rank)
{
	int ret;
	sqlite3_stmt *res;
	const char qnorank[] =
		"SELECT" ALL_PLAYER_COLUMN
		" FROM players"
		" WHERE name = ?";
	const char qrank[] =
		"SELECT" ALL_PLAYER_COLUMN "," RANK_COLUMN
		" FROM players"
		" WHERE name = ?";

	/*
	 * Reset player sets every fields to a 'no value' state, invalid for
	 * advanced use but still suitable for printing the player.
	 *
	 * That mean failing at every points of the function will leaves
	 * us with a player suitable for printing, as expected.
	 */
	reset_player(player, name);

	if (read_rank)
		ret = sqlite3_prepare_v2(db, qrank, sizeof(qrank), &res, NULL);
	else
		ret = sqlite3_prepare_v2(db, qnorank, sizeof(qnorank), &res, NULL);

	if (ret != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	ret = sqlite3_step(res);
	if (ret == SQLITE_DONE)
		goto not_found;
	else if (ret != SQLITE_ROW)
		goto fail;

	player_from_result_row(player, res, read_rank);

	sqlite3_finalize(res);
	return SUCCESS;

not_found:
	sqlite3_finalize(res);
	return NOT_FOUND;
fail:
	fprintf(stderr, "%s: read_player(%s): %s\n", config.dbpath, name, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return FAILURE;
}

int write_player(struct player *player)
{
	sqlite3_stmt *res;
	const char query[] =
		"INSERT OR REPLACE INTO players"
		" VALUES (?, ?, ?, ?, ?, ?)";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, player->name, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, player->clan, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 3, player->elo) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 4, player->lastseen) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 5, player->server_ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 6, player->server_port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return SUCCESS;

fail:
	fprintf(
		stderr, "%s: write_player(%s): %s\n",
		config.dbpath, player->name, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return FAILURE;
}

void set_clan(struct player *player, char *clan)
{
	assert(player != NULL);
	assert(clan != NULL);
	assert(is_valid_hexname(clan));

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

void record_elo_and_rank(struct player *player)
{
	sqlite3_stmt *res;
	const char query[] =
		"INSERT OR REPLACE INTO player_historic"
		" VALUES (?, ?, ?, ?)";

	assert(player != NULL);
	assert(player->elo != INVALID_ELO);
	assert(player->rank != UNRANKED);

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, player->name, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 2, time(NULL)) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 3, player->elo) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 4, player->rank) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return;

fail:
	fprintf(
		stderr, "%s: record_player_elo_and_rank(%s): %s\n",
		config.dbpath, player->name, sqlite3_errmsg(db));
	sqlite3_finalize(res);
}

unsigned count_players(void)
{
	unsigned retval;
	struct sqlite3_stmt *res;
	char query[] =
		"SELECT COUNT(1) FROM players";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_step(res) != SQLITE_ROW)
		goto fail;

	retval = sqlite3_column_int64(res, 0);

	sqlite3_finalize(res);
	return retval;

fail:
	fprintf(
		stderr, "%s: count_players(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}
