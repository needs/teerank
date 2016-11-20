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
		"SELECT COUNT(1) FROM servers WHERE" IS_VANILLA_CTF_SERVER;

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

int main_html_server_list(int argc, char **argv)
{
	int ret;
	unsigned offset;
	sqlite3_stmt *res;
	unsigned pnum;
	const char query[] =
		"SELECT" ALL_SERVER_COLUMN "," NUM_CLIENTS_COLUMN
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

	offset = (pnum - 1) * 100;

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 1, offset) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE && pnum > 1) {
		sqlite3_finalize(res);
		return EXIT_NOT_FOUND;
	}

	html_header(&CTF_TAB, "CTF", "/servers", NULL);
	print_section_tabs(SERVERS_TAB, NULL, NULL);
	html_start_server_list();

	while (ret == SQLITE_ROW) {
		struct server server;

		server_from_result_row(&server, res, 1);
		html_server_list_entry(++offset, &server);

		ret = sqlite3_step(res);
	}

	if (ret != SQLITE_DONE)
		goto fail;

	html_end_server_list();
	print_page_nav("/servers", pnum, get_npages());
	html_footer("server-list", relurl("/servers/%s.json?p=%u", argv[2], pnum));

	return EXIT_SUCCESS;

fail:
	fprintf(stderr, "%s: html_server_list(): %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
