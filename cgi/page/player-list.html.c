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
#include "index.h"
#include "page.h"

int page_player_list_html_main(int argc, char **argv)
{
	struct index_page ipage;
	struct indexed_player p;
	unsigned pnum;
	const char *indexname;
	const char *urlprefix;
	int ret;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-rank|by-lastseen\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	if (strcmp(argv[2], "by-rank") == 0) {
		indexname = "players_by_rank";
		urlprefix = "/players/by-rank";
	} else if (strcmp(argv[2], "by-lastseen") == 0) {
		indexname = "players_by_last_seen";
		urlprefix = "/players/by-lastseen";
	} else {
		fprintf(stderr, "%s: Should be either \"by-rank\" or \"by-lastseen\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	ret = open_index_page(
		indexname, &ipage, INDEX_DATA_INFOS_PLAYER,
		pnum, PLAYERS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	html_header(&CTF_TAB, "CTF", NULL);
	print_section_tabs(PLAYERS_TAB);

	html_start_player_list();

	while (index_page_foreach(&ipage, &p))
		html_player_list_entry(p.name, p.clan, p.elo, p.rank, *gmtime(&p.last_seen), 0);

	html_end_player_list();
	print_page_nav(urlprefix, &ipage);

	html_footer("player-list");

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
