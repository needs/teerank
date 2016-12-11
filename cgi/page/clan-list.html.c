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
#include "clan.h"

int main_html_clan_list(int argc, char **argv)
{
	unsigned pnum, offset, nrow;
	sqlite3_stmt *res;
	struct clan clan;

	const char *query =
		"SELECT" ALL_CLAN_COLUMNS
		" FROM players"
		" WHERE" IS_VALID_CLAN
		" GROUP BY clan"
		" ORDER BY nmembers DESC, clan"
		" LIMIT 100 OFFSET ?";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-nmembers\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_FAILURE;

	if (strcmp(argv[2], "by-nmembers") != 0) {
		fprintf(stderr, "%s: Should be \"by-nmembers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	html_header(&CTF_TAB, "CTF", "/clans", NULL);
	print_section_tabs(CLANS_TAB, NULL, NULL);
	html_start_clan_list();

	offset = (pnum - 1) * 100;
	foreach_clan(query, &clan, "u", offset)
		html_clan_list_entry(++offset, clan.name, clan.nmembers);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	html_end_clan_list();
	print_page_nav("/clans", pnum, count_clans() / 100 + 1);
	html_footer("clan-list", relurl("/clans/%s.json?p=%u", argv[2], pnum));

	return EXIT_SUCCESS;
}
