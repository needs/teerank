#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

DEFINE_SIMPLE_LIST_CLASS_FUNC(gametypes_class, "gametypelist");

void generate_html_gametypes(struct url *url)
{
	sqlite3_stmt *res;
	char *pnum_;
	unsigned nrow, pnum;
	url_t listurl;

	const char *qselect =
		"SELECT gametype,"
		"  COUNT(DISTINCT name) AS nplayers,"
		"  COUNT(DISTINCT clan) AS nclans,"
		"  (SELECT COUNT(1)"
		"   FROM servers"
		"   WHERE servers.gametype IS ranks.gametype) AS nservers,"
		"  COUNT(DISTINCT map) AS nmaps"
		" FROM ranks NATURAL JOIN players"
		" GROUP BY gametype"
		" ORDER BY nplayers DESC";

	const char *qcount =
		"SELECT COUNT(1)"
		" FROM ranks"
		" GROUP BY gametype";

	struct html_list_column cols[] = {
		{ "Gametype", NULL, HTML_COLTYPE_GAMETYPE },
		{ "Players" },
		{ "Clans" },
		{ "Servers" },
		{ "Maps" },
		{ NULL }
	};

	pnum_ = URL_EXTRACT(url, PARAM_PAGENUM(0));
	pnum = strtol(pnum_, NULL, 10);

	html_header(GAMETYPE_LIST_TAB);

	nrow = count_rows(qcount);
	res = foreach_init(qselect, "");
	URL(listurl, "/gametypes");
	html_list(res, cols, NULL, gametypes_class, listurl, pnum, nrow);

	html_footer(NULL, NULL);
}
