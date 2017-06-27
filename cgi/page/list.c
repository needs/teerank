#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "json.h"
#include "database.h"

static bool JSON = false;

/* Global context shared by every lists */
enum pcs pcs;
static char *gametype;
static char *map;
static char *order;
static unsigned pnum;

static void print_player_list(void)
{
	struct sqlite3_stmt *res;
	const char *sortby = NULL;

	char qselect[512], *qselectfmt =
		"SELECT rank, players.name, clan, elo,"
		"       lastseen, server_ip, server_port"
		" FROM players NATURAL JOIN ranks"
		" WHERE gametype = ? AND map = ?"
		" ORDER BY %s"
		" LIMIT 100 OFFSET %u";

	char *qcount =
		"SELECT COUNT(1)"
		" FROM players NATURAL JOIN ranks"
		" WHERE gametype = ? AND map = ?";

	if (strcmp(order, "rank") == 0)
		sortby = "rank";
	else if (strcmp(order, "lastseen") == 0)
		sortby = "lastseen DESC, rank";
	else
		error(400, "Invalid order \"%s\"", order);

	snprintf(qselect, sizeof(qselect), qselectfmt, sortby, (pnum - 1) * 100);
	res = foreach_init(qselect, "ss", gametype, map);

	if (JSON) {
		struct json_list_column cols[] = {
			{ "rank", "%u" },
			{ "name", "%s" },
			{ "clan", "%s" },
			{ "elo", "%i" },
			{ "lastseen", "%d" },
			{ NULL }
		};

		json("{%s:", "players");
		json_list(res, cols, "length");
		json("}");
	} else {
		url_t url;
		unsigned nrow;
		struct html_list_column cols[] = {
			{ "", NULL, HTML_COLTYPE_RANK },
			{ "Name", NULL, HTML_COLTYPE_PLAYER },
			{ "Clan", NULL, HTML_COLTYPE_CLAN },
			{ "Elo", "rank", HTML_COLTYPE_ELO },
			{ "Last seen", "lastseen", HTML_COLTYPE_LASTSEEN },
			{ NULL }
		};

		nrow = count_rows(qcount, "ss", gametype, map);
		URL(url, "/players", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_list(res, cols, order, "playerlist", url, pnum, nrow);
	}
}

static void print_clan_list(void)
{
	struct sqlite3_stmt *res;

	const char *qselect =
		"SELECT clan, CAST(AVG(elo) AS Int) AS avgelo, COUNT(1) AS nmembers"
		" FROM players NATURAL JOIN ranks"
		" WHERE clan IS NOT NULL"
		" GROUP BY clan"
		" ORDER BY avgelo DESC, nmembers DESC, clan"
		" LIMIT 100 OFFSET ?";

	const char *qcount =
		"SELECT COUNT(DISTINCT clan)"
		" FROM players"
		" WHERE clan IS NOT NULL";

	res = foreach_init(qselect, "u", (pnum - 1) * 100);

	if (JSON) {
		struct json_list_column cols[] = {
			{ "name", "%s" },
			{ "average_elo", "%i" },
			{ "nmembers", "%u" },
			{ NULL }
		};

		json("{%s:", "clans");
		json_list(res, cols, "length");
		json("}");
	} else {
		url_t url;
		unsigned nrow;
		struct html_list_column cols[] = {
			{ "Name", NULL, HTML_COLTYPE_CLAN },
			{ "Elo", NULL, HTML_COLTYPE_ELO },
			{ "Members", NULL },
			{ NULL }
		};

		nrow = count_rows(qcount);
		URL(url, "/clans", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_list(res, cols, NULL, "clanlist", url, pnum, nrow);
	}
}

static void print_server_list(void)
{
	struct sqlite3_stmt *res;

	const char *qselect =
		"SELECT name, ip, port, gametype, map,"
		" (SELECT COUNT(1)"
		"  FROM server_clients AS sc"
		"  WHERE sc.ip = servers.ip"
		"  AND sc.port = servers.port)"
		" AS num_clients, max_clients "
		" FROM servers"
		" WHERE gametype = ? AND map = ?"
		" ORDER BY num_clients DESC"
		" LIMIT 100 OFFSET ?";

	const char *qcount =
		"SELECT COUNT(1)"
		" FROM servers"
		" WHERE gametype = ? AND map = ?";

	res = foreach_init(qselect, "ssu", gametype, map, (pnum - 1) * 100);

	if (JSON) {
		struct json_list_column cols[] = {
			{ "name", "%s" },
			{ "ip", "%s" },
			{ "port", "%s" },
			{ "gametype", "%s" },
			{ "map", "%s" },
			{ "nplayers", "%u" },
			{ "maxplayers", "%u" },
			{ NULL }
		};

		json("{%s:", "servers");
		json_list(res, cols, "length");
		json("}");
	} else {
		url_t url;
		unsigned nrow;
		struct html_list_column cols[] = {
			{ "Name", NULL, HTML_COLTYPE_SERVER },
			{ "Gametype" },
			{ "Map" },
			{ "Players", NULL, HTML_COLTYPE_PLAYER_COUNT },
			{ NULL }
		};

		nrow = count_rows(qcount, "ss", gametype, map);
		URL(url, "/servers", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_list(res, cols, NULL, "serverlist", url, pnum, nrow);
	}
}

static void parse_list_args(struct url *url)
{
	char *pcs_, *pnum_;

	pcs_ = "players";
	pnum_    = URL_EXTRACT(url, PARAM_PAGENUM(0));
	gametype = URL_EXTRACT(url, PARAM_GAMETYPE(0));
	map      = URL_EXTRACT(url, PARAM_MAP(0));
	order    = URL_EXTRACT(url, PARAM_ORDER(0));

	if (url->ndirs > 0)
		pcs_ = url->dirs[0];

	if (strcmp(pcs_, "players") == 0)
		pcs = PCS_PLAYER;
	else if (strcmp(pcs_, "clans") == 0)
		pcs = PCS_CLAN;
	else if (strcmp(pcs_, "servers") == 0)
		pcs = PCS_SERVER;
	else
		error(400, "%s: Should be either \"players\", \"clans\" or \"servers\"\n", pcs_);

	pnum = parse_pnum(pnum_);
}

void generate_html_list(struct url *url)
{
	url_t urlfmt;
	char *subtab, *subclass;
	enum tab_type tab;

	JSON = false;
	parse_list_args(url);

	if (!*map) {
		subtab = "All maps";
		subclass = "allmaps";
	} else {
		subtab = map;
		subclass = NULL;
	}

	if (strcmp(gametype, "CTF") == 0)
		tab = CTF_TAB;
	else if (strcmp(gametype, "DM") == 0)
		tab = DM_TAB;
	else if (strcmp(gametype, "TDM") == 0)
		tab = TDM_TAB;
	else
		tab = GAMETYPE_TAB;

	URL(urlfmt, "maps", PARAM_GAMETYPE(gametype));
	html_header(
		tab,
		.title = gametype,
		.subtab = subtab,
		.subtab_class = subclass,
		.subtab_url = urlfmt);

	struct section_tab tabs[] = {
		{ "Players" }, { "Clans" }, { "Servers" }, { NULL }
	};

	URL(tabs[0].url, "/players", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
	URL(tabs[1].url, "/clans",   PARAM_GAMETYPE(gametype), PARAM_MAP(map));
	URL(tabs[2].url, "/servers", PARAM_GAMETYPE(gametype), PARAM_MAP(map));

	tabs[0].val = count_rows(
		"SELECT COUNT(1)"
		" FROM ranks"
		" WHERE gametype = ? AND map = ?",
		"ss", gametype, map
	);

	tabs[1].val = count_rows(
		"SELECT COUNT(DISTINCT clan)"
		" FROM players NATURAL JOIN ranks"
		" WHERE gametype = ? AND map = ?",
		"ss", gametype, map
	);

	tabs[2].val = count_rows(
		"SELECT COUNT(1)"
		" FROM servers"
		" WHERE gametype = ? AND map = ?",
		"ss", gametype, map
	);

	switch (pcs) {
	case PCS_PLAYER:
		tabs[0].active = true;
		print_section_tabs(tabs);
		print_player_list();
		URL(urlfmt, "/players.json");
		break;

	case PCS_CLAN:
		tabs[1].active = true;
		print_section_tabs(tabs);
		print_clan_list();
		URL(urlfmt, "/clans.json");
		break;

	case PCS_SERVER:
		tabs[2].active = true;
		print_section_tabs(tabs);
		print_server_list();
		URL(urlfmt, "/servers.json");
		break;
	}

	URL(
		urlfmt, urlfmt,
		PARAM_GAMETYPE(gametype),
		PARAM_MAP(map),
		PARAM_ORDER(order),
		PARAM_PAGENUM(pnum));

	html_footer("player-list", urlfmt);
}

void generate_json_list(struct url *url)
{
	JSON = true;

	parse_list_args(url);

	switch (pcs) {
	case PCS_PLAYER:
		print_player_list();
		break;
	case PCS_CLAN:
		print_clan_list();
		break;
	case PCS_SERVER:
		print_server_list();
		break;
	}
}
