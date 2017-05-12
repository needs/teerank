#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "player.h"
#include "clan.h"
#include "json.h"

int main_html_clan(struct url *url)
{
	unsigned nrow;
	sqlite3_stmt *res;
	char *cname;
	struct player p;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE clan = ? AND" IS_VALID_CLAN
		" ORDER BY" SORT_BY_RANK;

	cname = url->dirs[1];

	/* Eventually, print them */
	html_header(cname, cname, "/clans", NULL);
	html("<h2>%s</h2>", escape(cname));

	html_start_player_list(NULL, 0, NULL);

	foreach_player(query, &p, "s", cname)
		html_player_list_entry(&p, NULL, 1);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	html_end_player_list();
	html_footer("clan", relurl("/clans/%s.json", json_hexstring(cname)));

	return EXIT_SUCCESS;
}
