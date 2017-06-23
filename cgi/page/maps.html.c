#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

DEFINE_SIMPLE_LIST_CLASS_FUNC(maps_class, "maplist");

void generate_html_maps(struct url *url)
{
	sqlite3_stmt *res;
	enum tab_type tab;
	char *gametype, *pnum_;
	unsigned i, nrow, pnum;
	url_t listurl;

	const char *qselect =
		"SELECT map, gametype,"
		"  COUNT(DISTINCT name) AS nplayers,"
		"  COUNT(DISTINCT clan) AS nclans,"
		"  (SELECT COUNT(1)"
		"   FROM servers"
		"   WHERE servers.gametype IS ranks.gametype"
		"    AND servers.map IS IFNULL(ranks.map, servers.map)) AS nservers"
		" FROM ranks NATURAL JOIN players"
		" WHERE gametype IS ?"
		" GROUP BY map"
		" ORDER BY nplayers DESC";

	const char *qcount =
		"SELECT COUNT(1)"
		" FROM ranks"
		" WHERE gametype IS ?"
		" GROUP BY map";

	struct html_list_column cols[] = {
		{ "Map", NULL, HTML_COLTYPE_MAP },
		{ "Players" },
		{ "Clans" },
		{ "Servers" },
		{ NULL }
	};

	gametype = DEFAULT_PARAM_VALUE(PARAM_GAMETYPE(0));
	pnum_ = DEFAULT_PARAM_VALUE(PARAM_PAGENUM(0));

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "gametype") == 0)
			gametype = url->args[i].val;
		if (strcmp(url->args[i].name, "p") == 0)
			pnum_ = url->args[i].val;
	}

	pnum = strtol(pnum_, NULL, 10);

	if (strcmp(gametype, "CTF") == 0)
		tab = CTF_TAB;
	else if (strcmp(gametype, "DM") == 0)
		tab = DM_TAB;
	else if (strcmp(gametype, "TDM") == 0)
		tab = TDM_TAB;
	else
		tab = GAMETYPE_TAB;

	html_header(
		tab, .title = gametype,
		.subtab = "Map list",
		.subtab_class = "allmaps");

	nrow = count_rows(qcount, "s", gametype);
	res = foreach_init(qselect, "s", gametype);
	URL(listurl, "/maps", PARAM_GAMETYPE(gametype));
	html_list(res, cols, NULL, maps_class, listurl, pnum, nrow);

	html_footer(NULL, NULL);
}
