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

static int parse_pnum(const char *str, unsigned *pnum)
{
	long ret;

	errno = 0;
	ret = strtol(str, NULL, 10);
	if (ret == 0 && errno != 0)
		return perror(str), 0;

	/*
	 * Page numbers are unsigned but strtol() returns a long, so we
	 * need to make sure our page number fit into an unsigned.
	 */
	if (ret < 1) {
		fprintf(stderr, "%s: Must be positive\n", str);
		return 0;
	} else if (ret > UINT_MAX) {
		fprintf(stderr, "%s: Must lower than %u\n", str, UINT_MAX);
		return 0;
	}

	*pnum = ret;

	return 1;
}

static void html_start_clan_list(void)
{
	html("<table>");
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

static const unsigned CLANS_PER_PAGE = 100;

int page_clan_list_main(int argc, char **argv)
{
	struct index_page ipage;
	struct indexed_clan c;
	unsigned pnum, pos;
	int ret;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <page_number>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	ret = open_index_page(
		"clans_by_nmembers", &ipage, INDEX_DATA_INFOS_CLAN,
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
	print_page_nav("/clans/pages", &ipage);
	html_footer();

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
