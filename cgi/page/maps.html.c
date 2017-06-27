#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

void generate_html_maps(struct url *url)
{
	sqlite3_stmt *res;
	enum tab_type tab;
	char *gametype, *pnum_;
	unsigned nrow, pnum;
	url_t url_;

	const char *qselect =
		"SELECT map, gametype,"
		"  COUNT(DISTINCT name) AS nplayers,"
		"  COUNT(DISTINCT clan) AS nclans,"
		"  (SELECT COUNT(1)"
		"   FROM servers"
		"   WHERE servers.gametype = ranks.gametype"
		"    AND servers.map = ranks.map) AS nservers"
		" FROM ranks NATURAL JOIN players"
		" WHERE gametype = ?"
		" GROUP BY map"
		" ORDER BY nplayers DESC";

	const char *qcount =
		"SELECT COUNT(1)"
		" FROM ranks"
		" WHERE gametype = ?";

	struct html_list_column cols[] = {
		{ "Map", NULL, HTML_COLTYPE_MAP },
		{ "Players" },
		{ "Clans" },
		{ "Servers" },
		{ NULL }
	};

	gametype = URL_EXTRACT(url, PARAM_GAMETYPE(0));
	pnum_ = URL_EXTRACT(url, PARAM_PAGENUM(0));
	pnum = strtol(pnum_, NULL, 10);

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

	nrow = count_rows(qcount, "s", gametype);
	res = foreach_init(qselect, "s", gametype);
	URL(url_, "/maps", PARAM_GAMETYPE(gametype));
	html_list(res, cols, NULL, "maplist", url_, pnum, nrow);

	html_footer(NULL, NULL);
}
