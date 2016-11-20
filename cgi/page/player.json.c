#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "page.h"
#include "json.h"

static void json_player(struct player *player)
{
	printf("\"name\":\"%s\",", player->name);
	printf("\"clan\":\"%s\",", player->clan);
	printf("\"elo\":%d,", player->elo);
	printf("\"rank\":%u,", player->rank);
	printf("\"lastseen\":\"%s\",", json_date(player->lastseen));
	printf("\"server_ip\":\"%s\",", player->server_ip);
	printf("\"server_port\":\"%s\"", player->server_port);
}

static int json_player_historic(const char *pname)
{
	time_t epoch = 0;
	int ret;
	unsigned count = 0;
	sqlite3_stmt *res;
	char query[] =
		"SELECT timestamp, elo, rank"
		" FROM player_historic"
		" WHERE name = ?"
		" ORDER BY timestamp";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 1, pname, -1, NULL) != SQLITE_OK)
		goto fail;

	printf("\"historic\":{\"records\":[");

	while ((ret = sqlite3_step(res)) == SQLITE_ROW) {
		if (!count)
			epoch = sqlite3_column_int64(res, 1);

		printf(
			"[%ju, %d, %u],",
			(uintmax_t)(sqlite3_column_int64(res, 1) - epoch),
			sqlite3_column_int(res, 2),
			(unsigned)sqlite3_column_int64(res, 3));
		count++;
	}

	printf("],\"epoch\":%ju,\"length\":%u}", (uintmax_t)epoch, count);

	if (ret != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: json_player_historic(%s): %s\n",
		config.dbpath, pname, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

int main_json_player(int argc, char **argv)
{
	struct player player;
	int ret, full;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <player_name> full|short\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[2], "full") == 0)
		full = 1;
	else if (strcmp(argv[2], "short") == 0)
		full = 0;
	else {
		fprintf(stderr, "%s: Should be either \"full\" or \"short\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	if ((ret = read_player(&player, argv[1], 1)) != SUCCESS)
		return ret;

	putchar('{');

	json_player(&player);
	if (full && !json_player_historic(player.name))
		return EXIT_FAILURE;

	putchar('}');

	return EXIT_SUCCESS;
}
