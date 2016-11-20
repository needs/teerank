#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "cgi.h"
#include "page.h"
#include "player.h"
#include "json.h"

static void json_player(struct player *player)
{
	putchar('{');
	printf("\"name\":\"%s\",", player->name);
	printf("\"clan\":\"%s\",", player->clan);
	printf("\"elo\":%d,", player->elo);
	printf("\"rank\":%u,", player->rank);
	printf("\"lastseen\":\"%s\",", json_date(player->lastseen));
	printf("\"server_ip\":\"%s\",", player->server_ip);
	printf("\"server_port\":\"%s\"", player->server_port);
	putchar('}');
}

int main_json_player_list(int argc, char **argv)
{
	int ret;
	unsigned offset;
	sqlite3_stmt *res;
	const char *orderby1, *orderby2;
	unsigned pnum;
	char query[512], *queryfmt =
		"SELECT" ALL_PLAYER_COLUMN "," RANK_COLUMN
		" FROM players"
		" ORDER BY %s DESC, %s DESC, name DESC"
		" LIMIT 100 OFFSET %u";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-rank|by-lastseen\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	if (strcmp(argv[2], "by-rank") == 0) {
		orderby1 = "elo";
		orderby2 = "lastseen";

	} else if (strcmp(argv[2], "by-lastseen") == 0) {
		orderby1 = "lastseen";
		orderby2 = "elo";

	} else {
		fprintf(stderr, "%s: Should be either \"by-rank\" or \"by-lastseen\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	offset = (pnum - 1) * 100;
	snprintf(query, sizeof(query), queryfmt, orderby1, orderby2, offset);

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE && pnum > 1) {
		sqlite3_finalize(res);
		return EXIT_NOT_FOUND;
	}

	printf("{\"players\":[");

	while (ret == SQLITE_ROW) {
		struct player player;

		player_from_result_row(&player, res, 1);
		json_player(&player);
		offset++;

		if ((ret = sqlite3_step(res)) == SQLITE_ROW)
			putchar(',');
	}

	if (ret != SQLITE_DONE)
		goto fail;

	printf("],\"length\":%u}", offset - ((pnum - 1) * 100));

	sqlite3_finalize(res);
	return EXIT_SUCCESS;

fail:
	fprintf(
		stderr, "%s: json_clan_list(%s): %s\n",
		config.dbpath, argv[1], sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
