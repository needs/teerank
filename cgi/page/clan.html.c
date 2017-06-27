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
	sqlite3_stmt *res;
	char *cname;
	url_t urlfmt;

	const char *query =
		"SELECT name, clan, lastseen, server_ip, server_port"
		" FROM players"
		" WHERE clan IS NOT NULL AND clan = ?"
		" ORDER BY name";

	struct html_list_column cols[] = {
		{ "Name",      NULL,       HTML_COLTYPE_PLAYER },
		{ "Clan",      NULL,       HTML_COLTYPE_CLAN },
		{ "Last seen", "lastseen", HTML_COLTYPE_LASTSEEN },
		{ NULL }
	};

	if (!(cname = URL_EXTRACT(url, PARAM_NAME(0))))
		error(400, "Clan name required");

	html_header(CUSTOM_TAB, .title = "Clan", .subtab = cname);
	html("<h2>%s</h2>", cname);

	res = foreach_init(query, "s", cname);
	html_list(res, cols, .class = "clanplayerlist");

	URL(urlfmt, "/clan.json", PARAM_NAME(cname));
	html_footer("clan", urlfmt);
}
