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
		{ "Gametype", "gametype", HTML_COLTYPE_GAMETYPE },
		{ "Players", "players" },
		{ "Clans", "clans" },
		{ "Servers", "servers" },
		{ "Maps", "maps" },
		{ NULL }
	};

	pnum = strtol(URL_EXTRACT(url, PARAM_PAGENUM(0)), NULL, 10);
	order = URL_EXTRACT(url, PARAM("sort", "players", "%s", NULL));
	check_order(order, "gametype", "players", "clans", "servers", "maps");

	html_header(GAMETYPE_LIST_TAB);

	URL(listurl, "/gametypes");
	list = init_list(qselect, qcount, 100, pnum, order, NULL);
	html_list(&list, cols, NULL, "gametypelist", listurl);

	html_footer(NULL, NULL);
}
