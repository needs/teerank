#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

void generate_html_maps(struct url *url)
{
	struct list list;
	enum tab_type tab;
	char *gametype, *order;
	unsigned pnum;
	url_t url_;

	const char *qselect =
		"SELECT map, gametype,"

		" (SELECT COUNT(1)"
		"  FROM ranks"
		"  WHERE ranks.gametype = ?1"
		"  AND ranks.map = foo.map)"
		" AS nplayers,"

		" (SELECT COUNT(DISTINCT clan)"
		"  FROM ranks NATURAL JOIN players"
		"  WHERE ranks.gametype = ?1"
		"  AND ranks.map = foo.map"
		"  AND clan <> '')"
		" AS nclans,"

		" (SELECT COUNT(1)"
		"  FROM servers"
		"  WHERE servers.gametype = ?1"
		"  AND (foo.map = '' OR servers.map = foo.map))"
		" AS nservers"

		" FROM ("
		"  SELECT map, gametype from ranks UNION"
		"  SELECT map, gametype from servers"
		" ) AS foo"

		" WHERE gametype = ?1"
		" GROUP BY map"
		" ORDER BY %s, nplayers DESC, nservers DESC";

	const char *qcount =
		"SELECT COUNT(DISTINCT map)"
		" FROM ("
		"  SELECT map, gametype from ranks UNION"
		"  SELECT map, gametype from servers"
		" ) AS foo"
		" WHERE gametype = ?";

	struct list_order orders[] = {
		{ "players", "nplayers DESC" },
		{ "map", "map" },
		{ "clans", "nclans DESC" },
		{ "servers", "nservers DESC" },
		{ NULL }
	};

	struct html_list_column cols[] = {
		{ "Map", HTML_COLTYPE_MAP, "map" },
		{ "Players", HTML_COLTYPE_NPLAYERS, "players" },
		{ "Clans", HTML_COLTYPE_NCLANS, "clans"},
		{ "Servers", HTML_COLTYPE_NSERVERS, "servers" },
		{ NULL }
	};

	gametype = URL_EXTRACT(url, PARAM_GAMETYPE(0));
	pnum = strtol(URL_EXTRACT(url, PARAM_PAGENUM(0)), NULL, 10);
	order = URL_EXTRACT(url, PARAM_ORDER(0));

	if (strcmp(gametype, "CTF") == 0)
		tab = CTF_TAB;
	else if (strcmp(gametype, "DM") == 0)
		tab = DM_TAB;
	else if (strcmp(gametype, "TDM") == 0)
		tab = TDM_TAB;
	else
		tab = GAMETYPE_TAB;

	URL(url_, "/players", PARAM_GAMETYPE(gametype));
	html_header(
		tab, .title = gametype,
		.subtab = "Map list",
		.subtab_class = "allmaps",
		.subtab_url = url_);

	URL(url_, "/maps", PARAM_GAMETYPE(gametype));
	list = init_list(qselect, qcount, 100, pnum, orders, order, "s", gametype);
	html_list(&list, cols, NULL, "maplist", url_);

	html_footer(NULL, NULL);
}
