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

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

static void print_nav(unsigned pnum, unsigned npages)
{
	/* Number of pages shown before and after the current page */
	static const unsigned EXTRA_PAGES = 3;
	unsigned i;

	assert(pnum <= npages);

	html("<nav class=\"pages\">");
	if (pnum == 1)
		html("<a class=\"previous\">Previous</a>");
	else
		html("<a class=\"previous\" href=\"/pages/%u.html\">Previous</a>",
		       pnum - 1);

	if (pnum > EXTRA_PAGES + 1)
		html("<a href=\"/pages/1.html\">1</a>");
	if (pnum > EXTRA_PAGES + 2)
		html("<span>...</span>");

	for (i = min(EXTRA_PAGES, pnum - 1); i > 0; i--)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       pnum - i, pnum - i);

	html("<a class=\"current\">%u</a>", pnum);

	for (i = 1; i <= min(EXTRA_PAGES, npages - pnum); i++)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       pnum + i, pnum + i);

	if (pnum + EXTRA_PAGES + 1 < npages)
		html("<span>...</span>");
	if (pnum + EXTRA_PAGES < npages)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       npages, npages);

	if (pnum == npages)
		html("<a class=\"next\">Next</a>");
	else
		html("<a class=\"next\" href=\"/pages/%u.html\">Next</a>",
		       pnum + 1);
	html("</nav>");
}

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
	print_nav(pnum, ipage.npages);
	html_footer();

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
