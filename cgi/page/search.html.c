#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#include "config.h"
#include "cgi.h"
#include "html.h"
#include "player.h"
#include "server.h"
#include "clan.h"

/* Too many results is meaningless */
#define MAX_RESULTS 50

struct clan {
	char *name;
	unsigned nmembers;
};

static void read_clan_result(void *data, sqlite3_stmt *res)
{
	struct clan *clan = data;

	snprintf(clan->name, sizeof(clan->name), "%s", sqlite3_column_text(res, 0));
	clan->nmembers = sqlite3_column_int64(res, 1);
}

static void read_player_result(void *data, sqlite3_stmt *res)
{
	struct player *player = data;
	player_from_result_row(player, res, 1);
}

static void read_server_result(void *data, sqlite3_stmt *res)
{
	struct server *server = data;
	server_from_result_row(server, res, 1);
}

static void start_player_list(void)
{
	html_start_player_list(1, 1, 0);
}

static void print_player(unsigned pos, void *data)
{
	struct player *p = data;
	html_player_list_entry(
		p->name, p->clan, p->elo, p->rank, p->lastseen,
		build_addr(p->server_ip, p->server_port), 0);
}
static void print_clan(unsigned pos, void *data)
{
	struct clan *clan = data;
	html_clan_list_entry(pos, clan->name, clan->nmembers);
}
static void print_server(unsigned pos, void *data)
{
	html_server_list_entry(pos, data);
}

/*
 * The following compute the relevance of a string in a query, using the
 * following prioprities: exact match, prefix, suffix, anything else.
 */
#define IS_RELEVANT(col) \
	" " col " LIKE '%' || ? || '%' "
#define RELEVANCE(col) \
	" CASE" \
	"  WHEN " col " LIKE ? THEN 0" \
	"  WHEN " col " LIKE ? || '%' THEN 1" \
	"  WHEN " col " LIKE '%' || ? THEN 2" \
	"  ELSE 3" \
	" END "

struct search_info {
	void (*start_list)(void);
	void (*end_list)(void);
	void (*read_result)(void *data, sqlite3_stmt *res);
	void (*print_result)(unsigned pos, void *data);
	const char *emptylist;

	enum section_tab tab;
	const char *sprefix;

	const char *count_query;
	const char *search_query;
};

static const struct search_info PLAYER_SINFO = {
	start_player_list,
	html_end_player_list,
	read_player_result,
	print_player,
	"No players found",

	PLAYERS_TAB,
	"/players",

	"SELECT COUNT(1)"
	" FROM players"
	" WHERE" IS_RELEVANT("name")
	" LIMIT ?",

	"SELECT" ALL_PLAYER_COLUMN "," RANK_COLUMN
	" FROM players"
	" WHERE" IS_RELEVANT("name")
	" ORDER BY" RELEVANCE("name") ", elo"
	" LIMIT ?"
};

static const struct search_info CLAN_SINFO = {
	html_start_clan_list,
	html_end_clan_list,
	read_clan_result,
	print_clan,
	"No clans found",

	CLANS_TAB,
	"/clans",

	"SELECT COUNT(1)"
	" FROM players"
	" WHERE" IS_VALID_CLAN "AND" IS_RELEVANT("clan")
	" LIMIT ?",

	"SELECT clan, COUNT(1) AS nmembers"
	" FROM players"
	" WHERE" IS_VALID_CLAN "AND" IS_RELEVANT("clan")
	" GROUP BY clan"
	" ORDER BY" RELEVANCE("clan") ", nmembers"
	" LIMIT ?"
};

static const struct search_info SERVER_SINFO = {
	html_start_server_list,
	html_end_server_list,
	read_server_result,
	print_server,
	"No servers found",

	SERVERS_TAB,
	"/servers",

	"SELECT COUNT(1)"
	" FROM servers"
	" WHERE" IS_VANILLA_CTF_SERVER "AND" IS_RELEVANT("name")
	" LIMIT ?",

	"SELECT" ALL_SERVER_COLUMN "," NUM_CLIENTS_COLUMN
	" FROM servers"
	" WHERE" IS_VANILLA_CTF_SERVER "AND" IS_RELEVANT("name")
	" ORDER BY" RELEVANCE("name") ", num_clients"
	" LIMIT ?"
};

static unsigned count_results(const char *query, const char *value)
{
	sqlite3_stmt *res;
	unsigned nresults = 0;

	/* Ignore failure, those data doesn't really matter anyway */

	if (sqlite3_prepare_v2(db, query, -1, &res, NULL) != SQLITE_OK)
		goto out;
	if (sqlite3_bind_text(res, 1, value, -1, NULL) != SQLITE_OK)
		goto out;
	if (sqlite3_bind_int(res, 2, MAX_RESULTS) != SQLITE_OK)
		goto out;

	if (sqlite3_step(res) == SQLITE_ROW)
		nresults = sqlite3_column_int64(res, 0);

out:
	sqlite3_finalize(res);
	return nresults;
}

static int search(const struct search_info *sinfo, const char *value)
{
	int ret;
	unsigned count = 0;
	sqlite3_stmt *res;
	union {
		struct player player;
		struct clan clan;
		struct server server;
	} result;

	if (sqlite3_prepare_v2(db, sinfo->search_query, -1, &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, value, -1, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, value, -1, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 3, value, -1, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 4, value, -1, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 5, MAX_RESULTS) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE) {
		html("%s", sinfo->emptylist);
		sqlite3_finalize(res);
		return 1;
	}

	sinfo->start_list();

	while (ret == SQLITE_ROW) {
		sinfo->read_result(&result, res);
		sinfo->print_result(count, &result);
		ret = sqlite3_step(res);
		count++;
	}

	sinfo->end_list();

	if (ret != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: search(%s): %s\n",
		config.dbpath, value, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

int main_html_search(int argc, char **argv)
{
	unsigned tabvals[SECTION_TABS_COUNT];
	const struct search_info *sinfo, **s, *sinfos[] = {
		&PLAYER_SINFO, &CLAN_SINFO, &SERVER_SINFO, NULL
	};

	if (argc != 3) {
		fprintf(stderr, "usage: %s players|clans|servers <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "players") == 0)
		sinfo = &PLAYER_SINFO;
	else if (strcmp(argv[1], "clans") == 0)
		sinfo = &CLAN_SINFO;
	else if (strcmp(argv[1], "servers") == 0)
		sinfo = &SERVER_SINFO;
	else {
		fprintf(stderr, "%s: Should be either \"players\", \"clans\" or \"servers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	for (s = sinfos; *s; s++)
		tabvals[(*s)->tab] = count_results((*s)->count_query, argv[2]);

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, "Search results", sinfo->sprefix, argv[2]);
	print_section_tabs(sinfo->tab, argv[2], tabvals);

	if (!search(sinfo, argv[2]))
		return EXIT_FAILURE;

	html_footer(NULL, NULL);

	return EXIT_SUCCESS;
}
