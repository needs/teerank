#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "cgi.h"
#include "clan.h"
#include "json.h"

int main_json_clan_list(int argc, char **argv)
{
	unsigned pnum, offset, nrow;
	sqlite3_stmt *res;
	struct clan clan;

	const char query[] =
		"SELECT" ALL_CLAN_COLUMNS
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


	printf("{\"clans\":[");

	offset = (pnum - 1) * 100;
	foreach_clan(query, &clan, "u", offset) {
		if (nrow)
			putchar(',');

		putchar('{');
		printf("\"name\":\"%s\",", json_hexstring(clan.name));
		printf("\"nmembers\":\"%u\"", clan.nmembers);
		putchar('}');
	}

	printf("],\"length\":%u}", nrow);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	return EXIT_SUCCESS;
}
