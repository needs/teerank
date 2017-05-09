#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "database.h"
#include "player.h"

static const struct order {
	char *sortby, *urlprefix;
} BY_RANK = {
	SORT_BY_RANK, "/players"
}, BY_LASTSEEN = {
	SORT_BY_LASTSEEN, "/players/by-lastseen"
};

int main_html_player_list(int argc, char **argv)
{
	char *map, *gametype;
	const struct order *order;
	unsigned pnum, offset;
	struct player p;

	struct sqlite3_stmt *res;
	unsigned nrow;

	char query[512], *queryfmt =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE gametype = ? AND map LIKE ?"
		" ORDER BY %s"
		" LIMIT 100 OFFSET %u";

	if (argc != 5) {
		fprintf(stderr, "usage: %s <gametype> <map> rank|lastseen <page_number>\n", argv[0]);
		return EXIT_FAILURE;
	}

	gametype = argv[1];
	map = argv[2];

	if (strcmp(argv[3], "rank") == 0) {
		order = &BY_RANK;
	} else if (strcmp(argv[3], "lastseen") == 0) {
		order = &BY_LASTSEEN;
	} else {
		fprintf(stderr, "%s: Should be either \"rank\" or \"lastseen\"\n", argv[3]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[4], &pnum))
		return EXIT_FAILURE;

	if (strcmp(gametype, "CTF") == 0)
		html_header(&CTF_TAB, "CTF", "/players", NULL);
	else if (strcmp(gametype, "DM") == 0)
		html_header(&DM_TAB, "DM", "/players", NULL);
	else if (strcmp(gametype, "TDM") == 0)
		html_header(&TDM_TAB, "TDM", "/players", NULL);
	else
		html_header(gametype, gametype, "/players", NULL);

	/* Map is unused for now */
	(void)map;

	print_section_tabs(PLAYERS_TAB, NULL, NULL);

	if (order == &BY_RANK)
		html_start_player_list(1, 0, pnum);
	else
		html_start_player_list(0, 1, pnum);

	offset = (pnum - 1) * 100;
	snprintf(query, sizeof(query), queryfmt, order->sortby, offset);

	foreach_player(query, &p, "ss", gametype, map)
		html_player_list_entry(&p, NULL, 0);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	html_end_player_list();
	print_page_nav(order->urlprefix, pnum, count_ranked_players("", "") / 100 + 1);
	html_footer("player-list", relurl("/players/%s.json?p=%u", argv[2], pnum));

	return EXIT_SUCCESS;
}
