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

static void html_start_clan_list(void)
{
	html("<table class=\"clanlist\">");
	html("<thead>");
	html("<tr>");
	html("<th></th>");
	html("<th>Name</th>");
	html("<th>Members</th>");
	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

static void html_end_clan_list(void)
{
	html("</tbody>");
	html("</table>");
}

static void html_clan_list_entry(
	unsigned pos, const char *hexname, unsigned nmembers)
{
	char name[NAME_LENGTH];

	assert(hexname != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	hexname_to_name(hexname, name);
	html("<td><a href=\"/clans/%s.html\">%s</a></td>", hexname, escape(name));

	/* Members */
	html("<td>%u</td>", nmembers);

	html("</tr>");
}

int page_clan_list_html_main(int argc, char **argv)
{
	struct index_page ipage;
	struct indexed_clan c;
	unsigned pnum, pos;
	const char *indexname;
	int ret;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-nmembers\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	if (strcmp(argv[2], "by-nmembers") == 0)
		indexname = "clans_by_nmembers";
	else {
		fprintf(stderr, "%s: Should be \"by-nmembers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	ret = open_index_page(
		indexname, &ipage, INDEX_DATA_INFOS_CLAN,
		pnum, CLANS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	html_header(&CTF_TAB, "CTF", NULL);
	print_section_tabs(CLANS_TAB);

	html_start_clan_list();

	while ((pos = index_page_foreach(&ipage, &c)))
		html_clan_list_entry(pos, c.name, c.nmembers);

	html_end_clan_list();
	print_page_nav("/clans/by-nmembers", &ipage);

	html_footer("clan-list");

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
