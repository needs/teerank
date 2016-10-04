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
	struct jfile jfile;
	struct indexed_player *p;

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
		indexname = "players_by_lastseen";
	else {
		fprintf(stderr, "%s: Should be either \"by-rank\" or \"by-lastseen\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	ret = open_index_page(
		indexname, &ipage, &INDEX_DATA_INFO_PLAYER,
		pnum, PLAYERS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	json_init(&jfile, stdout, "<stdout>");

	json_write_object_start(&jfile, NULL);
	json_write_unsigned(&jfile, "length", ipage.plen);
	json_write_array_start(&jfile, "players");

	while ((p = index_page_foreach(&ipage, NULL))) {
		json_write_object_start(&jfile, NULL);

		json_write_string(&jfile, "name", p->name, sizeof(p->name));
		json_write_string(&jfile, "clan", p->clan, sizeof(p->clan));
		json_write_int(&jfile, "elo", p->elo);
		json_write_unsigned(&jfile, "rank", p->rank);
		json_write_time(&jfile, "lastseen", p->lastseen);

		json_write_string(&jfile, "server_ip", p->server_ip, sizeof(p->server_ip));
		json_write_string(&jfile, "server_port", p->server_port, sizeof(p->server_port));

		json_write_object_end(&jfile);
	}

	json_write_object_end(&jfile);
	json_write_array_end(&jfile);

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
