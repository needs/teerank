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
		indexname, &ipage, &INDEX_DATA_INFO_CLAN,
		pnum, CLANS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	html_header(&CTF_TAB, "CTF", "/clans", NULL);
	print_section_tabs(CLANS_TAB, NULL, NULL);

	html_start_clan_list();

	while ((pos = index_page_foreach(&ipage, &c)))
		html_clan_list_entry(pos, c.name, c.nmembers);

	html_end_clan_list();
	print_page_nav("/clans/by-nmembers", &ipage);

	html_footer("clan-list");

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
