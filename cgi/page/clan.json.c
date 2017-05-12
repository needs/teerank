#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "player.h"
#include "json.h"
#include "clan.h"

int main_json_clan(struct url *url)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct player p;
	char *cname;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE clan = ? AND " IS_VALID_CLAN
		" ORDER BY" SORT_BY_RANK;

	cname = url->dirs[1];

	printf("{\"members\":[");

	foreach_player(query, &p, "s", cname) {
		if (nrow)
			putchar(',');
		printf("\"%s\"", json_hexstring(p.name));
	}

	printf("],\"nmembers\":%u}", nrow);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	return EXIT_SUCCESS;
}
