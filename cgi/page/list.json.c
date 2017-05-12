#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "player.h"
#include "clan.h"
#include "server.h"
#include "json.h"

/* Global context shared by every lists */
static char *gametype;
static char *map;
static char *order;
static unsigned pnum;

static int print_player_list(void)
{
	unsigned nrow;
	sqlite3_stmt *res;
	const char *sortby;
	struct player p;

	char query[512], *queryfmt =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE gametype = ? AND map = ?"
		" ORDER BY %s"
		" LIMIT 100 OFFSET %u";

	if (strcmp(order, "rank") == 0)
		sortby = SORT_BY_RANK;
	else if (strcmp(order, "lastseen") == 0)
		sortby = SORT_BY_LASTSEEN;
	else {
		fprintf(stderr, "%s: Should be either \"rank\" or \"lastseen\"\n", order);
		return EXIT_FAILURE;
	}

	snprintf(query, sizeof(query), queryfmt, sortby, (pnum - 1) * 100);

	printf("{\"players\":[");

	foreach_player(query, &p, "ss", gametype, map) {
		if (nrow)
			putchar(',');

		putchar('{');
		printf("\"name\":\"%s\",", json_hexstring(p.name));
		printf("\"clan\":\"%s\",", json_hexstring(p.clan));
		printf("\"elo\":%d,", p.elo);
		printf("\"rank\":%u,", p.rank);
		printf("\"lastseen\":\"%s\",", json_date(p.lastseen));
		printf("\"server_ip\":\"%s\",", p.server_ip);
		printf("\"server_port\":\"%s\"", p.server_port);
		putchar('}');
	}

	printf("],\"length\":%u}", nrow);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	return EXIT_SUCCESS;
}

static int print_clan_list(void)
{
	unsigned offset, nrow;
	sqlite3_stmt *res;
	struct clan clan;

	const char *query =
		"SELECT" ALL_CLAN_COLUMNS
		" FROM players"
		" WHERE" IS_VALID_CLAN
		" GROUP BY clan"
		" ORDER BY nmembers DESC, clan"
		" LIMIT 100 OFFSET ?";

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

static int print_server_list(void)
{
	unsigned nrow, offset;
	sqlite3_stmt *res;
	struct server server;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE" IS_VANILLA_CTF_SERVER
		" ORDER BY num_clients DESC"
		" LIMIT 100 OFFSET ?";

	offset = (pnum - 1) * 100;

	printf("{\"servers\":[");

	foreach_extended_server(query, &server, "u", offset) {
		if (nrow)
			putchar(',');

		putchar('{');
		printf("\"ip\":\"%s\",", server.ip);
		printf("\"port\":\"%s\",", server.port);
		printf("\"name\":\"%s\",", json_escape(server.name));
		printf("\"gametype\":\"%s\",", json_escape(server.gametype));
		printf("\"map\":\"%s\",", json_escape(server.map));

		printf("\"maxplayers\":%u,", server.max_clients);
		printf("\"nplayers\":%u", server.num_clients);
		putchar('}');
	}

	printf("],\"length\":%u}", nrow);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	return EXIT_SUCCESS;
}

int main_json_list(struct url *url)
{
	enum pcs pcs;
	int ret;

	ret = parse_list_args(url, &pcs, &gametype, &map, &order, &pnum);
	if (ret != EXIT_SUCCESS)
		return ret;

	switch (pcs) {
	case PCS_PLAYER:
		return print_player_list();
	case PCS_CLAN:
		return print_clan_list();
	case PCS_SERVER:
		return print_server_list();
	}

	return EXIT_FAILURE;
}
