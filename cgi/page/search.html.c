#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

/* Too many results is meaningless */
#define MAX_RESULTS 50

/*
 * This conditional evaluate the relevance of a string, using the
 * following order: exact match, prefix, suffix, anything else.
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
	list_class_func_t class;
	struct html_list_column *cols;

	int tab;
	const char *sprefix;

	const char *count_query;
	const char *search_query;
};

DEFINE_SIMPLE_LIST_CLASS_FUNC(player_list_class, "search-playerlist");
DEFINE_SIMPLE_LIST_CLASS_FUNC(clan_list_class,   "clanlist");
DEFINE_SIMPLE_LIST_CLASS_FUNC(server_list_class, "serverlist");

static const struct search_info PLAYER_SINFO = {
	player_list_class,
	(struct html_list_column[]) {
		{ "Name",      NULL, HTML_COLTYPE_PLAYER },
		{ "Clan",      NULL, HTML_COLTYPE_CLAN },
		{ "Last seen", NULL, HTML_COLTYPE_LASTSEEN },
		{ NULL }
	},

	0,
	"/players",

	"SELECT COUNT(1)"
	" FROM players"
	" WHERE" IS_RELEVANT("name")
	" LIMIT ?",

	"SELECT name, clan, lastseen, server_ip, server_port"
	" FROM players"
	" WHERE" IS_RELEVANT("name")
	" ORDER BY" RELEVANCE("name") ", clan"
	" LIMIT ?"
};

static const struct search_info CLAN_SINFO = {
	clan_list_class,
	(struct html_list_column[]) {
		{ "Name",    NULL, HTML_COLTYPE_CLAN },
		{ "Members", NULL },
		{ NULL }
	},

	1,
	"/clans",

	"SELECT COUNT(1)"
	" FROM players"
	" WHERE clan IS NOT NULL AND" IS_RELEVANT("clan")
	" GROUP BY clan"
	" LIMIT ?",

	"SELECT clan, COUNT(1) AS nmembers"
	" FROM players"
	" WHERE clan IS NOT NULL AND" IS_RELEVANT("clan")
	" GROUP BY clan"
	" ORDER BY" RELEVANCE("clan") ", nmembers"
	" LIMIT ?"
};

static const struct search_info SERVER_SINFO = {
	server_list_class,
	(struct html_list_column[]) {
		{ "Name" , NULL, HTML_COLTYPE_SERVER },
		{ "Gametype" },
		{ "Map" },
		{ "Players", NULL, HTML_COLTYPE_PLAYER_COUNT },
		{ NULL }
	},

	2,
	"/servers",

	"SELECT COUNT(1)"
	" FROM servers"
	" WHERE" IS_RELEVANT("name")
	" LIMIT ?",

	"SELECT name, ip, port, gametype, map,"
	" (SELECT COUNT(1)"
	"  FROM server_clients AS sc"
	"  WHERE sc.ip = servers.ip"
	"  AND sc.port = servers.port) AS num_clients, max_clients"
	" FROM servers"
	" WHERE" IS_RELEVANT("name")
	" ORDER BY" RELEVANCE("name") ", num_clients"
	" LIMIT ?"
};

void generate_html_search(struct url *url)
{
	sqlite3_stmt *res;
	char *squery = NULL;
	char *pcs;
	unsigned i;

	const struct search_info *sinfo = NULL, **s, *sinfos[] = {
		&PLAYER_SINFO, &CLAN_SINFO, &SERVER_SINFO, NULL
	};

	struct section_tab tabs[] = {
		{ "Players" }, { "Clans" }, { "Servers" }, { NULL }
	};

	if (url->ndirs == 2)
		pcs = url->dirs[1];
	else
		pcs = "players";

	if (strcmp(pcs, "players") == 0)
		sinfo = &PLAYER_SINFO;
	else if (strcmp(pcs, "clans") == 0)
		sinfo = &CLAN_SINFO;
	else if (strcmp(pcs, "servers") == 0)
		sinfo = &SERVER_SINFO;
	else
		error(400, "%s: Should be either \"players\", \"clans\" or \"servers\"", pcs);

	for (i = 0; i < url->nargs; i++)
		if (strcmp(url->args[i].name, "q") == 0)
			squery = url->args[i].val ? url->args[i].val : "";

	if (!squery)
		error(400, "Missing 'q' parameter\n");

	for (s = sinfos; *s; s++)
		tabs[(*s)->tab].val = count_rows((*s)->count_query, "si", squery, MAX_RESULTS);

	html_header("Search results", "Search results", sinfo->sprefix, squery);

	URL(tabs[0].url, "/search/players", PARAM_SQUERY(squery));
	URL(tabs[1].url, "/search/clans",   PARAM_SQUERY(squery));
	URL(tabs[2].url, "/search/servers", PARAM_SQUERY(squery));

	print_section_tabs(tabs);
	res = foreach_init(
		sinfo->search_query, "ssssi",
		squery, squery, squery, squery, MAX_RESULTS);
	html_list(res, sinfo->cols, "", sinfo->class, NULL, 1, 1);
	html_footer(NULL, NULL);
}
