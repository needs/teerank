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

/*
 * This conditional evaluate the relevance of a string, using the
 * following order: exact match, prefix, suffix, anything else.
 *
 * We double '%' because the query will be passed to snprintf() when
 * calling init_list().
 */
#define IS_RELEVANT(col) \
	" " col " LIKE '%%' || ?1 || '%%' "
#define RELEVANCE(col) \
	" CASE" \
	"  WHEN " col " LIKE ?1 THEN 0" \
	"  WHEN " col " LIKE ?1 || '%%' THEN 1" \
	"  WHEN " col " LIKE '%%' || ?1 THEN 2" \
	"  ELSE 3" \
	" END "

struct search_info {
	char *class;
	struct html_list_column *cols;

	int tab;
	char *search_url;

	const char *qcount;
	const char *qselect;
};

static const struct search_info PLAYER_SINFO = {
	"search-playerlist",
	(struct html_list_column[]) {
		{ "Name",      HTML_COLTYPE_PLAYER },
		{ "Clan",      HTML_COLTYPE_CLAN },
		{ "Last seen", HTML_COLTYPE_LASTSEEN },
		{ NULL }
	},

	0,
	"/search",

	"SELECT COUNT(1)"
	" FROM players"
	" WHERE" IS_RELEVANT("name"),

	"SELECT name, clan, lastseen"
	" FROM players"
	" WHERE" IS_RELEVANT("name")
	" ORDER BY" RELEVANCE("name") ", clan"
};

static const struct search_info CLAN_SINFO = {
	"clanlist",
	(struct html_list_column[]) {
		{ "Name",    HTML_COLTYPE_CLAN },
		{ "Members" },
		{ NULL }
	},

	1,
	"/search/clans",

	"SELECT COUNT(DISTINCT clan)"
	" FROM players"
	" WHERE clan <> '' AND" IS_RELEVANT("clan"),

	"SELECT clan, COUNT(1) AS nmembers"
	" FROM players"
	" WHERE clan <> '' AND" IS_RELEVANT("clan")
	" GROUP BY clan"
	" ORDER BY" RELEVANCE("clan") ", nmembers"
};

#define COUNT_SERVER_CLIENTS(ip, port)                                  \

static const struct search_info SERVER_SINFO = {
	"serverlist",
	(struct html_list_column[]) {
		{ "Name" , HTML_COLTYPE_SERVER },
		{ "Gametype" },
		{ "Map" },
		{ "Players", HTML_COLTYPE_PLAYER_COUNT },
		{ NULL }
	},

	2,
	"/search/servers",

	"SELECT COUNT(1)"
	" FROM servers"
	" WHERE" IS_RELEVANT("name"),

	"SELECT name, ip, port, gametype, map,"
	" (SELECT COUNT(1)"
	"  FROM server_clients"
	"  WHERE server_clients.ip = servers.ip"
	"  AND server_clients.port = servers.port)"
	" AS num_clients, max_clients"
	" FROM servers"
	" WHERE" IS_RELEVANT("name")
	" ORDER BY" RELEVANCE("name") ", num_clients"
};

void generate_html_search(struct url *url)
{
	struct list list;
	unsigned pnum;
	char *squery;
	char *pcs;

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

	squery = URL_EXTRACT(url, PARAM_SQUERY(0));
	pnum = strtol(URL_EXTRACT(url, PARAM_PAGENUM(0)), NULL, 10);

	for (s = sinfos; *s; s++)
		tabs[(*s)->tab].val = count_rows((*s)->qcount, "s", squery);

	html_header(
		CUSTOM_TAB,
		.title = "Search",
		.subtab = squery,
		.search_url = sinfo->search_url,
		.search_query = squery);

	URL(tabs[0].url, "/search", PARAM_SQUERY(squery));
	URL(tabs[1].url, "/search/clans",   PARAM_SQUERY(squery));
	URL(tabs[2].url, "/search/servers", PARAM_SQUERY(squery));
	tabs[sinfo->tab].active = true;

	print_section_tabs(tabs);
	list = init_list(
		sinfo->qselect, sinfo->qcount, 100, pnum, NULL, NULL, "s", squery);
	html_list(&list, sinfo->cols, .class = sinfo->class, .url = tabs[sinfo->tab].url);
	html_footer(NULL, NULL);
}
