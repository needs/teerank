#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "page.h"

static const struct order {
	char *column1, *column2, *urlprefix;
} BY_RANK = {
	"elo", "lastseen", "/players"
}, BY_LASTSEEN = {
	"lastseen", "elo", "/players/by-lastseen"
};

int main_html_player_list(int argc, char **argv)
{
	const struct order *order;
	unsigned pnum, offset;
	int ret;

	struct sqlite3_stmt *res;
	char query[512], *queryfmt =
		"SELECT" ALL_PLAYER_COLUMN "," RANK_COLUMN
		" FROM players"
		" ORDER BY %s DESC, %s DESC, name DESC"
		" LIMIT 100 OFFSET %u";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-rank|by-lastseen\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	if (strcmp(argv[2], "by-rank") == 0) {
		order = &BY_RANK;
	} else if (strcmp(argv[2], "by-lastseen") == 0) {
		order = &BY_LASTSEEN;
	} else {
		fprintf(stderr, "%s: Should be either \"by-rank\" or \"by-lastseen\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	offset = (pnum - 1) * 100;
	snprintf(
		query, sizeof(query), queryfmt,
		order->column1, order->column2, offset);

	if (sqlite3_prepare_v2(db, query, -1, &res, NULL) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE && pnum > 1) {
		sqlite3_finalize(res);
		return EXIT_NOT_FOUND;
	}

	html_header(&CTF_TAB, "CTF", "/players", NULL);
	print_section_tabs(PLAYERS_TAB, NULL, NULL);

	if (order == &BY_RANK)
		html_start_player_list(1, 0, pnum);
	else
		html_start_player_list(0, 1, pnum);

	while (ret == SQLITE_ROW) {
		struct player p;

		player_from_result_row(&p, res, 1);

		html_player_list_entry(
			p.name, p.clan, p.elo, p.rank, p.lastseen,
			build_addr(p.server_ip, p.server_port), 0);
		ret = sqlite3_step(res);
	}

	if (ret != SQLITE_DONE)
		goto fail;

	html_end_player_list();
	print_page_nav(order->urlprefix, pnum, count_players() / 100 + 1);
	html_footer("player-list", relurl("/players/%s.json?p=%u", argv[2], pnum));

	sqlite3_finalize(res);
	return EXIT_SUCCESS;

fail:
	fprintf(stderr, "%s: html_player_list: %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 1;
}
