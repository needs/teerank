#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

void generate_html_maps(struct url *url)
{
	struct list list;
	enum tab_type tab;
	char *gametype, *pnum_;
	unsigned pnum;
	url_t url_;

	const char *qselect =
		"SELECT map, gametype,"

		" (SELECT COUNT(1)"
		"  FROM ranks AS r"
		"  WHERE r.gametype = ?1"
		"  AND r.map = ranks.map)"
		" AS nplayers,"

		" (SELECT COUNT(DISTINCT clan)"
		"  FROM ranks AS r NATURAL JOIN players AS p"
		"  WHERE r.gametype = ?1"
		"  AND r.map = ranks.map"
		"  AND clan <> '')"
		" AS nclans,"

		" (SELECT COUNT(1)"
		"  FROM servers"
		"  WHERE servers.gametype = ?1"
		"  AND (ranks.map = '' OR servers.map = ranks.map))"
		" AS nservers"

		" FROM ranks"
		" WHERE gametype = ?1"
		" GROUP BY map"
		" ORDER BY nplayers DESC";

	const char *qcount =
		"SELECT COUNT(DISTINCT map)"
		" FROM ranks NATURAL JOIN players"
		" WHERE gametype = ?";

	struct html_list_column cols[] = {
		{ "Map", HTML_COLTYPE_MAP },
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

	URL(url_, "/maps", PARAM_GAMETYPE(gametype));
	list = init_list(qselect, qcount, 100, pnum, NULL, NULL, "s", gametype);
	html_list(&list, cols, NULL, "maplist", url_);

	html_footer(NULL, NULL);
}
