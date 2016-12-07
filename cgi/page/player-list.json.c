#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "cgi.h"
#include "player.h"
#include "json.h"

static void json_player(struct player *player)
{
	putchar('{');
	printf("\"name\":\"%s\",", json_escape(player->name));
	printf("\"clan\":\"%s\",", json_escape(player->clan));
	printf("\"elo\":%d,", player->elo);
	printf("\"rank\":%u,", player->rank);
	printf("\"lastseen\":\"%s\",", json_date(player->lastseen));
	printf("\"server_ip\":\"%s\",", player->server_ip);
	printf("\"server_port\":\"%s\"", player->server_port);
	putchar('}');
}

int main_json_player_list(int argc, char **argv)
{
	unsigned nrow, offset;
	sqlite3_stmt *res;
	const char *sortby;
	unsigned pnum;
	struct player p;

	char query[512], *queryfmt =
		"SELECT" ALL_EXTENDED_PLAYER_COLUMNS
		" FROM players"
		" ORDER BY %s"
		" LIMIT 100 OFFSET %u";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <page_number> by-rank|by-lastseen\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	if (strcmp(argv[2], "by-rank") == 0) {
		sortby = SORT_BY_ELO;

	} else if (strcmp(argv[2], "by-lastseen") == 0) {
		sortby = SORT_BY_LASTSEEN;

	} else {
		fprintf(stderr, "%s: Should be either \"by-rank\" or \"by-lastseen\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	offset = (pnum - 1) * 100;
	snprintf(query, sizeof(query), queryfmt, sortby, offset);

	printf("{\"players\":[");

	foreach_extended_player(query, &p) {
		if (nrow)
			putchar(',');

		json_player(&p);
	}

	printf("],\"length\":%u}", nrow);

	return EXIT_SUCCESS;
}
