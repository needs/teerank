#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"

void generate_html_clan(struct url *url)
{
	struct list list;
	char *cname;
	url_t urlfmt;

	if (!(cname = URL_EXTRACT(url, PARAM_NAME(0))))
		error(400, "Clan name required");

	html_header(CUSTOM_TAB, .title = "Clan", .subtab = cname);
	html("<h2>%s</h2>", cname);

	html("<h3>Best ranking</h3>");

	list = init_simple_list(
		"SELECT gametype,"

		" (SELECT CAST(AVG(elo) AS Int)"
		"  FROM ranks AS r NATURAL JOIN players AS p"
		"  WHERE r.gametype = ranks.gametype"
		"   AND r.map = ''"
		"   AND p.clan = players.clan"
		"  ORDER BY r.rank"
		"  LIMIT 10) AS avgelo,"

		" COUNT(name) AS ranked,"
		" MAX(lastseen) AS max_lastseen"

		" FROM ranks NATURAL JOIN players"
		" WHERE clan = ?1 AND map = ''"
		" GROUP BY gametype"
		" ORDER BY CASE"
		"  WHEN ranked >= 10 THEN 0"
		"  ELSE 10 - ranked"
		" END, avgelo DESC, ranked DESC, gametype"
		" LIMIT 5",

		"s", cname
	);

	struct html_list_column cols1[] = {
		{ "Gametype", HTML_COLTYPE_GAMETYPE },
		{ "Elo", HTML_COLTYPE_ELO },
		{ "Members", 0 },
		{ "Activity", HTML_COLTYPE_LASTSEEN },
		{ NULL }
	};

	html_list(&list, cols1, .class = "clanlist");

	html("<h3>Members</h3>");
	list = init_simple_list(
		"SELECT name, clan, lastseen"
		" FROM players"
		" WHERE clan <> '' AND clan = ?"
		" ORDER BY name",
		"s", cname
	);

	struct html_list_column cols2[] = {
		{ "Name", HTML_COLTYPE_PLAYER },
		{ "Clan", HTML_COLTYPE_CLAN },
		{ "Last seen", HTML_COLTYPE_LASTSEEN },
		{ NULL }
	};

	html_list(&list, cols2, .class = "clanplayerlist");

	URL(urlfmt, "/clan.json", PARAM_NAME(cname));
	html_footer("clan", urlfmt);
}
