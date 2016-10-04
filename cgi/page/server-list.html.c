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

int page_server_list_html_main(int argc, char **argv)
{
	struct index_page ipage;
	struct indexed_server *server;
	unsigned pnum, pos;
	const char *indexname;
	int ret;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-nplayers\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	if (strcmp(argv[2], "by-nplayers") == 0)
		indexname = "servers_by_nplayers";
	else {
		fprintf(stderr, "%s: Should be \"by-nplayers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	ret = open_index_page(
		indexname, &ipage, &INDEX_DATA_INFO_SERVER,
		pnum, SERVERS_PER_PAGE);
	if (ret != SUCCESS)
		return ret;

	html_header(&CTF_TAB, "CTF", "/servers", NULL);
	print_section_tabs(SERVERS_TAB, NULL, NULL);

	html_start_server_list();

	while ((server = index_page_foreach(&ipage, &pos)))
		html_server_list_entry(pos, server);

	html_end_server_list();
	print_page_nav("/servers/by-nplayers", &ipage);

	html_footer("server-list", relurl("/servers/%s.json?p=%u", argv[2], pnum));

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
