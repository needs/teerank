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
	struct list list;

	char *qselect =
		"SELECT rank, players.name, clan, elo, lastseen"
		" FROM players NATURAL JOIN ranks"
		" WHERE gametype = ? AND map = ?"
		" ORDER BY %s";

	char *qcount =
		"SELECT COUNT(1)"
		" FROM players NATURAL JOIN ranks"
		" WHERE gametype = ? AND map = ?";

	struct list_order orders[] = {
		{ "rank", "rank" },
		{ "lastseen", "lastseen DESC, rank" },
		{ NULL }
	};

	list = init_list(qselect, qcount, 100, pnum, orders, order, "ss", gametype, map);

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
		json_list(&list, cols, "length");
		json("}");
	} else {
		url_t url;
		struct html_list_column cols[] = {
			{ "", HTML_COLTYPE_RANK },
			{ "Name", HTML_COLTYPE_PLAYER },
			{ "Clan", HTML_COLTYPE_CLAN },
			{ "Elo", HTML_COLTYPE_ELO, "rank" },
			{ "Last seen", HTML_COLTYPE_LASTSEEN, "lastseen" },
			{ NULL }
		};

		URL(url, "/players", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_list(&list, cols, order, "playerlist", url);
	}
}

static void print_clan_list(void)
{
	struct list list;


	const char *qselect =
		"SELECT clan,"

		/*
		 * Only the 10 best players contribute to the average
		 * elo so that you can't lower one'es average by just
		 * creating new players with the same clan tag.
		 */
		" (SELECT CAST(AVG(elo) AS Int)"
		"  FROM ranks AS r NATURAL JOIN players AS p"
		"  WHERE r.gametype = ?1"
		"   AND r.map = IFNULL(?2, map)"
		"   AND p.clan = players.clan"
		"  ORDER BY r.rank"
		"  LIMIT 10) AS avgelo,"

		" COUNT(name) AS ranked,"
		" (SELECT COUNT(DISTINCT name)"
		"  FROM players AS p"
		"  WHERE p.clan = players.clan) AS total"

		" FROM ranks NATURAL JOIN players"
		" WHERE clan <> '' AND gametype = ?1 AND map = IFNULL(?2, map)"
		" GROUP BY clan"
		" ORDER BY %s, avgelo DESC, ranked DESC, clan";

	const char *qcount =
		"SELECT COUNT(DISTINCT clan)"
		" FROM players NATURAL JOIN ranks"
		" WHERE clan <> '' AND gametype = ? AND map = IFNULL(?, map)";

	/*
	 * We want clan with more than 10 ranked players to all be
	 * sorted on the average elo of their ten best players.
	 * However, clan with less than 10 ranked players will be ranked
	 * lower even if the average is higher.
	 */
	char *elo_order_by =
		"CASE"
		" WHEN ranked >= 10 THEN 0"
		" ELSE 10 - ranked"
		" END";

	struct list_order orders[] = {
		{ "elo", elo_order_by },
		{ "members", "ranked DESC" },
		{ "name", "clan" },
		{ NULL }
	};

	list = init_list(qselect, qcount, 100, pnum, orders, order, "ss", gametype, map);

	if (JSON) {
		struct json_list_column cols[] = {
			{ "name", "%s" },
			{ "average_elo", "%i" },
			{ "ranked_members", "%u" },
			{ "total_members", "%u" },
			{ NULL }
		};

		json("{%s:", "clans");
		json_list(&list, cols, "length");
		json("}");
	} else {
		url_t url;
		struct html_list_column cols[] = {
			{ "Name", HTML_COLTYPE_CLAN, "name" },
			{ "Elo", HTML_COLTYPE_ELO, "elo" },
			{ "Members", HTML_COLTYPE_NMEMBERS, "members" },
			{ NULL }
		};

		URL(url, "/clans", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_list(&list, cols, NULL, "clanlist", url);
	}
}

static void print_server_list(void)
{
	struct list list;
	char *map_ = *map ? map : NULL;

	const char *qselect =
		"SELECT name, ip, port, gametype, map,"
		" (SELECT COUNT(1)"
		"  FROM server_clients AS sc"
		"  WHERE sc.ip = servers.ip"
		"  AND sc.port = servers.port)"
		" AS num_clients, max_clients "
		" FROM servers"
		" WHERE gametype = ? AND map = IFNULL(?, map)"
		" ORDER BY %s, num_clients DESC, name";

	const char *qcount =
		"SELECT COUNT(1)"
		" FROM servers"
		" WHERE gametype = ? AND map = IFNULL(?, map)";

	struct list_order orders[] = {
		{ "players", "num_clients DESC" },
		{ "name", "name" },
		{ "map", "map" },
		{ NULL }
	};

	list = init_list(qselect, qcount, 100, pnum, orders, order, "ss", gametype, map_);

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
		json_list(&list, cols, "length");
		json("}");
	} else {
		url_t url;
		struct html_list_column cols[] = {
			{ "Name", HTML_COLTYPE_SERVER, "name" },
			{ "Gametype" },
			{ "Map", 0, "map" },
			{ "Players", HTML_COLTYPE_PLAYER_COUNT, "players" },
			{ NULL }
		};

		URL(url, "/servers", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_list(&list, cols, NULL, "serverlist", url);
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
		" WHERE clan <> '' AND gametype = ? AND map = ?",
		"ss", gametype, map
	);

	tabs[2].val = count_rows(
		"SELECT COUNT(1)"
		" FROM servers"
		" WHERE gametype = ?1 AND (?2 = '' OR map = ?2)",
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
