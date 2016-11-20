#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "page.h"
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

int main_json_clan(int argc, char **argv)
{
	int ret;
	unsigned count = 0;
	sqlite3_stmt *res;
	const char query[] =
		"SELECT" ALL_PLAYER_COLUMN "," RANK_COLUMN
		" FROM players"
		" WHERE clan = ? AND " IS_VALID_CLAN
		" ORDER BY" SORT_BY_ELO;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 1, argv[1], -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE)
		goto not_found;

	printf("{\"members\":[");

	while (ret == SQLITE_ROW) {
		struct player player;

		player_from_result_row(&player, res, 1);
		json_player(&player);

		count++;
		ret = sqlite3_step(res);
	}

	printf("],\"nmembers\":%u}", count);

	if (ret != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return EXIT_SUCCESS;

not_found:
	sqlite3_finalize(res);
	return EXIT_NOT_FOUND;

fail:
	fprintf(
		stderr, "%s: json_clan(%s): %s\n",
		config.dbpath, argv[1], sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
