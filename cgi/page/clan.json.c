#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "player.h"
#include "html.h"
#include "clan.h"

int main_json_clan(struct url *url)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	struct player p;
	char *cname = NULL;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE clan = ? AND " IS_VALID_CLAN
		" ORDER BY" SORT_BY_RANK;

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			cname = url->args[i].val;
	}

	json("{%s:[", "members");

	foreach_player(query, &p, "s", cname) {
		if (nrow)
			json(",");
		json("%s", p.name);
	}

	json("],%s:%u}", "nmembers", nrow);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	return EXIT_SUCCESS;
}
