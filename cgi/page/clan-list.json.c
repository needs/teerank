#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "cgi.h"
#include "page.h"
#include "player.h"
#include "clan.h"

int main_json_clan_list(int argc, char **argv)
{
	int ret;
	unsigned count = 0;
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
		fprintf(stderr, "usage: %s <pnum> by-nmembers\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 1, (pnum - 1) * 100) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE && pnum > 1) {
		sqlite3_finalize(res);
		return EXIT_NOT_FOUND;
	}

	printf("{\"clans\":[");

	while (ret == SQLITE_ROW) {
		if (count)
			putchar(',');
		putchar('{');
		printf("\"name\":\"%s\",", sqlite3_column_text(res, 0));
		printf("\"nmembers\":\"%u\"", (unsigned)sqlite3_column_int64(res, 1));
		putchar('}');

		ret = sqlite3_step(res);
		count++;
	}

	if (ret != SQLITE_DONE)
		goto fail;

	printf("],\"length\":%u}", count);

	sqlite3_finalize(res);
	return EXIT_SUCCESS;

fail:
	fprintf(
		stderr, "%s: json_clan_list(%s): %s\n",
		config.dbpath, argv[1], sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
