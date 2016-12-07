#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "json.h"
#include "clan.h"

int main_json_clan(int argc, char **argv)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct player p;

	const char *query =
		"SELECT" ALL_EXTENDED_PLAYER_COLUMNS
		" FROM players"
		" WHERE clan = ? AND " IS_VALID_CLAN
		" ORDER BY" SORT_BY_ELO;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	printf("{\"members\":[");

	foreach_extended_player(query, &p, "s", argv[1]) {
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
