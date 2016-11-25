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
#include "database.h"
#include "player.h"

static const struct order {
	char *sortby, *urlprefix;
} BY_RANK = {
	SORT_BY_ELO, "/players"
}, BY_LASTSEEN = {
	SORT_BY_LASTSEEN, "/players/by-lastseen"
};

int main_html_player_list(int argc, char **argv)
{
	const struct order *order;
	unsigned pnum, offset;
	struct player p;

	struct sqlite3_stmt *res;
	unsigned nrow;

	char query[512], *queryfmt =
		"SELECT" ALL_EXTENDED_PLAYER_COLUMNS
		" FROM players"
		" ORDER BY %s"
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

	html_header(&CTF_TAB, "CTF", "/players", NULL);
	print_section_tabs(PLAYERS_TAB, NULL, NULL);

	if (order == &BY_RANK)
		html_start_player_list(1, 0, pnum);
	else
		html_start_player_list(0, 1, pnum);

	offset = (pnum - 1) * 100;
	snprintf(query, sizeof(query), queryfmt, order->sortby, offset);

	foreach_extended_player(query, &p)
		html_player_list_entry(&p, NULL, 0);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	html_end_player_list();
	print_page_nav(order->urlprefix, pnum, count_players() / 100 + 1);
	html_footer("player-list", relurl("/players/%s.json?p=%u", argv[2], pnum));

	return EXIT_SUCCESS;
}
