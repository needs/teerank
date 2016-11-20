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
#include "page.h"

static unsigned get_npages(void)
{
	unsigned npages;
	struct sqlite3_stmt *res;
	char query[] =
		"SELECT COUNT(1) FROM (SELECT DISTINCT clan FROM players WHERE" IS_VALID_CLAN ")";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_ROW)
		goto fail;

	npages = sqlite3_column_int64(res, 0) / 100 + 1;

	sqlite3_finalize(res);
	return npages;

fail:
	fprintf(stderr, "%s: get_npages(): %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 1;
}

int main_html_clan_list(int argc, char **argv)
{
	int ret;
	unsigned offset;
	sqlite3_stmt *res;
	unsigned pnum;
	const char query[] =
		"SELECT clan, COUNT(1) AS nmembers"
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

	offset = (pnum - 1) * 100;

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 1, offset) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE && pnum > 1) {
		sqlite3_finalize(res);
		return EXIT_NOT_FOUND;
	}

	html_header(&CTF_TAB, "CTF", "/clans", NULL);
	print_section_tabs(CLANS_TAB, NULL, NULL);
	html_start_clan_list();

	while (ret == SQLITE_ROW) {
		html_clan_list_entry(
			++offset,
			(const char*)sqlite3_column_text(res, 0),
			sqlite3_column_int(res, 1));

		ret = sqlite3_step(res);
	}

	if (ret != SQLITE_DONE)
		goto fail;

	html_end_clan_list();
	print_page_nav("/clans", pnum, get_npages());
	html_footer("clan-list", relurl("/clans/%s.json?p=%u", argv[2], pnum));

	return EXIT_SUCCESS;

fail:
	fprintf(stderr, "%s: html_clan_list(): %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
