#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "player.h"
#include "clan.h"

int main_html_clan(int argc, char **argv)
{
	unsigned nrow;
	sqlite3_stmt *res;
	char cname[NAME_LENGTH];
	struct player p;

	const char query[] =
		"SELECT" ALL_EXTENDED_PLAYER_COLUMNS
		" FROM players"
		" WHERE clan = ? AND" IS_VALID_CLAN
		" ORDER BY" SORT_BY_ELO;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Eventually, print them */
	hexname_to_name(argv[1], cname);
	CUSTOM_TAB.name = escape(cname);
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, cname, "/clans", NULL);
	html("<h2>%s</h2>", escape(cname));

	html_start_player_list(1, 1, 0);

	foreach_extended_player(query, &p, "s", argv[1])
		html_player_list_entry(
			p.name, p.clan, p.elo, p.rank, p.lastseen,
			build_addr(p.server_ip, p.server_port), 1);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	html_end_player_list();
	html_footer("clan", relurl("/clans/%s.json", argv[1]));

	return EXIT_SUCCESS;
}
