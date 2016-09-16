#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "cgi.h"
#include "index.h"
#include "page.h"

int page_player_list_json_main(int argc, char **argv)
{
	struct index_page ipage;

	const char *indexname;
	unsigned pnum;
	int ret;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-rank|by-lastseen\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	if (strcmp(argv[2], "by-rank") == 0)
		indexname = "players_by_rank";
	else if (strcmp(argv[2], "by-lastseen") == 0)
		indexname = "players_by_last_seen";
	else {
		fprintf(stderr, "%s: Should be either \"by-rank\" or \"by-lastseen\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	ret = open_index_page(
		indexname, &ipage, INDEX_DATA_INFO_PLAYER,
		pnum, PLAYERS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	printf("{\"length\":%u,\"players\":[", ipage.plen);

	if (!index_page_dump_all(&ipage))
		return EXIT_FAILURE;

	printf("]}");

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
