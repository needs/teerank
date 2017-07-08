#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

void generate_html_gametypes(struct url *url)
{
	struct list list;
	char *order;
	unsigned pnum;
	url_t listurl;

	const char *qselect =
		"SELECT gametype,"

		" (SELECT COUNT(DISTINCT name)"
		"  FROM ranks AS r"
		"  WHERE r.gametype = ranks.gametype)"
		" AS nplayers,"

		" (SELECT COUNT(DISTINCT clan)"
		"  FROM ranks AS r NATURAL JOIN players AS p"
		"  WHERE r.gametype = ranks.gametype"
		"  AND clan <> '')"
		" AS nclans,"

		" (SELECT COUNT(1)"
		"  FROM servers"
		"  WHERE servers.gametype = ranks.gametype)"
		" AS nservers,"

		" COUNT(DISTINCT map) AS nmaps"
		" FROM ranks"
		" WHERE map <> ''"
		" GROUP BY gametype"
		" ORDER BY %s, nplayers DESC";

	const char *qcount =
		"SELECT COUNT(DISTINCT gametype)"
		" FROM ranks";

	struct html_list_column cols[] = {
		{ "Gametype", HTML_COLTYPE_GAMETYPE, "gametype" },
		{ "Players", HTML_COLTYPE_NPLAYERS, "players" },
		{ "Clans", HTML_COLTYPE_NCLANS, "clans" },
		{ "Servers", HTML_COLTYPE_NSERVERS, "servers" },
		{ "Maps", HTML_COLTYPE_NMAPS, "maps" },
		{ NULL }
	};

	struct list_order orders[] = {
		{ "players", "nplayers DESC" },
		{ "gametype", "gametype" },
		{ "clans", "nclans DESC" },
		{ "servers", "nservers DESC" },
		{ "maps", "nmaps DESC" },
		{ NULL }
	};

	pnum = strtol(URL_EXTRACT(url, PARAM_PAGENUM(0)), NULL, 10);
	order = URL_EXTRACT(url, PARAM_ORDER(0));

	html_header(GAMETYPE_LIST_TAB);

	URL(listurl, "/gametypes");
	list = init_list(qselect, qcount, 100, pnum, orders, order, NULL);
	html_list(&list, cols, NULL, "gametypelist", listurl);

	html_footer(NULL, NULL);
}
