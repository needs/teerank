#include <stdlib.h>
#include <stdio.h>

#include "clan.h"
#include "config.h"

unsigned count_clans(void)
{
	unsigned retval;
	struct sqlite3_stmt *res;

	/*
	 * Turns out 'SELECT DISTINCT ...' is slower than 'SELECT
	 * COUNT(1) FROM (SELECT DISTINCT ...)'.
	 */
	char query[] =
		"SELECT COUNT(1)"
		" FROM"
		" (SELECT DISTINCT clan"
		"  FROM players"
		"  WHERE" IS_VALID_CLAN ")";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_ROW)
		goto fail;

	retval = sqlite3_column_int64(res, 0);

	sqlite3_finalize(res);
	return retval;

fail:
	fprintf(stderr, "%s: count_clans(): %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}
