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

int main_html_clan(int argc, char **argv)
{
	int ret;
	sqlite3_stmt *res;
	char clan_name[NAME_LENGTH];
	const char query[] =
		"SELECT" ALL_PLAYER_COLUMN "," RANK_COLUMN
		" FROM players"
		" WHERE clan = ? AND" IS_VALID_CLAN
		" ORDER BY" SORT_BY_ELO;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Load players */
	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 1, argv[1], -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE)
		goto not_found;

	/* Eventually, print them */
	hexname_to_name(argv[1], clan_name);
	CUSTOM_TAB.name = escape(clan_name);
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, clan_name, "/clans", NULL);
	html("<h2>%s</h2>", escape(clan_name));

	html_start_player_list(1, 1, 0);

	while (ret == SQLITE_ROW) {
		struct player p;

		player_from_result_row(&p, res, 1),

		html_player_list_entry(
			p.name, p.clan, p.elo, p.rank, p.lastseen,
			build_addr(p.server_ip, p.server_port), 1);

		ret = sqlite3_step(res);
	}

	if (ret != SQLITE_DONE)
		goto fail;

	html_end_player_list();
	html_footer("clan", relurl("/clans/%s.json", argv[1]));

	sqlite3_finalize(res);
	return EXIT_SUCCESS;

not_found:
	sqlite3_finalize(res);
	return EXIT_NOT_FOUND;

fail:
	fprintf(
		stderr, "%s: html_clan(%s): %s\n",
		config.dbpath, argv[1], sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
