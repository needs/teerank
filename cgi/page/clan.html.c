#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"

DEFINE_SIMPLE_LIST_CLASS_FUNC(player_list_class, "clanplayerlist");

void generate_html_clan(struct url *url)
{
	sqlite3_stmt *res;
	char *cname = NULL;
	url_t urlfmt;
	unsigned i;

	const char *query =
		"SELECT name, clan, lastseen, server_ip, server_port"
		" FROM players"
		" WHERE clan IS NOT NULL AND clan IS ?"
		" ORDER BY name";

	struct html_list_column cols[] = {
		{ "Name",      NULL,       HTML_COLTYPE_PLAYER },
		{ "Clan",      NULL,       HTML_COLTYPE_CLAN },
		{ "Last seen", "lastseen", HTML_COLTYPE_LASTSEEN },
		{ NULL }
	};

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			cname = url->args[i].val;
	}

	/* Eventually, print them */
	html_header(CUSTOM_TAB, .title = "Clan", .subtab = cname);
	html("<h2>%s</h2>", cname);

	res = foreach_init(query, "s", cname);
	html_list(res, cols, "", player_list_class, NULL, 0, 0);

	URL(urlfmt, "/clan.json", PARAM_NAME(cname));
	html_footer("clan", urlfmt);
}
