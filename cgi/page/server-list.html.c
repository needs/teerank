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
#include "server.h"

int main_html_server_list(int argc, char **argv)
{
	struct server server;
	unsigned pnum, offset, nrow;
	sqlite3_stmt *res;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE" IS_VANILLA_CTF_SERVER
		" ORDER BY num_clients DESC"
		" LIMIT 100 OFFSET ?";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-nplayers\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	if (strcmp(argv[2], "by-nplayers") != 0) {
		fprintf(stderr, "%s: Should be \"by-nplayers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	html_header(&CTF_TAB, "CTF", "/servers", NULL);
	print_section_tabs(SERVERS_TAB, NULL, NULL);

	html_start_server_list();

	offset = (pnum - 1) * 100;
	foreach_extended_server(query, &server, "u", offset)
		html_server_list_entry(++offset, &server);

	html_end_server_list();

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	print_page_nav("/servers", pnum, count_vanilla_servers() / 100 + 1);
	html_footer("server-list", relurl("/servers/%s.json?p=%u", argv[2], pnum));

	return EXIT_SUCCESS;
}
