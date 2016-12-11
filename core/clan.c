#include <stdlib.h>
#include <stdio.h>

#include "clan.h"
#include "teerank.h"

void read_clan(sqlite3_stmt *res, void *_c)
{
	struct clan *c = _c;

	snprintf(c->name, sizeof(c->name), "%s", sqlite3_column_text(res, 0));
	c->nmembers = sqlite3_column_int64(res, 1);
}

unsigned count_clans(void)
{
	/*
	 * Turns out 'SELECT DISTINCT ...' is slower than 'SELECT
	 * COUNT(1) FROM (SELECT DISTINCT ...)'.
	 */
	const char *query =
		"SELECT COUNT(1)"
		" FROM"
		" (SELECT DISTINCT clan"
		"  FROM players"
		"  WHERE" IS_VALID_CLAN ")";

	return count_rows(query);
}
