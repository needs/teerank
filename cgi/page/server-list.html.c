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

static void html_start_server_list(void)
{
	html("<table class=\"serverlist\">");
	html("<thead>");
	html("<tr>");
	html("<th></th>");
	html("<th>Name</th>");
	html("<th>Gametype</th>");
	html("<th>Map</th>");
	html("<th>Players</th>");
	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

static void html_end_server_list(void)
{
	html("</tbody>");
	html("</table>");
}

static void html_server_list_entry(unsigned pos, struct indexed_server *server)
{
	assert(server != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	html("<td>%s</td>", server->name);

	/* Gametype */
	html("<td>%s</td>", server->gametype);

	/* Map */
	html("<td>%s</td>", server->map);

	/* Players */
	html("<td>%u / %u</td>", server->nplayers, server->maxplayers);

	html("</tr>");
}

int page_server_list_html_main(int argc, char **argv)
{
	struct index_page ipage;
	struct indexed_server server;
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
		indexname, &ipage, INDEX_DATA_INFO_SERVER,
		pnum, SERVERS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	html_header(&CTF_TAB, "CTF", NULL);
	print_section_tabs(SERVERS_TAB);

	html_start_server_list();

	while ((pos = index_page_foreach(&ipage, &server)))
		html_server_list_entry(pos, &server);

	html_end_server_list();
	print_page_nav("/servers/by-nplayers", &ipage);

	html_footer("server-list");

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
