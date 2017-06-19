#include <stdlib.h>
#include <stdio.h>

#include "clan.h"
#include "teerank.h"
#include "database.h"

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
		"  WHERE clan IS NOT NULL)";

	return count_rows(query);
}
