#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "index.h"
#include "page.h"

int page_server_list_json_main(int argc, char **argv)
{
	struct index_page ipage;
	struct jfile jfile;
	struct indexed_server s;

	unsigned pnum;
	int ret;

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	ret = open_index_page(
		"servers_by_nplayers", &ipage, &INDEX_DATA_INFO_SERVER,
		pnum, SERVERS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	json_init(&jfile, stdout, "<stdout>");

	json_write_object_start(&jfile, NULL);
	json_write_unsigned(&jfile, "length", ipage.plen);
	json_write_array_start(&jfile, "servers");

	while (index_page_foreach(&ipage, &s)) {
		json_write_object_start(&jfile, NULL);

		json_write_string(&jfile, "name", s.name, sizeof(s.name));
		json_write_string(&jfile, "gametype", s.gametype, sizeof(s.gametype));
		json_write_string(&jfile, "map", s.map, sizeof(s.map));

		json_write_unsigned(&jfile, "nplayers", s.nplayers);
		json_write_unsigned(&jfile, "maxplayers", s.maxplayers);

		json_write_object_end(&jfile);
	}

	json_write_object_end(&jfile);
	json_write_array_end(&jfile);

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
