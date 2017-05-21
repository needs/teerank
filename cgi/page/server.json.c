#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "server.h"
#include "json.h"

static void json_server(struct server *server)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct client c;

	const char *query =
		"SELECT" ALL_SERVER_CLIENT_COLUMNS
		" FROM server_clients"
		" WHERE ip = ? AND port = ?"
		" ORDER BY" SORT_BY_SCORE;

	putchar('{');
	printf("\"ip\":\"%s\",", server->ip);
	printf("\"port\":\"%s\",", server->port);

	printf("\"name\":\"%s\",", json_escape(server->name));
	printf("\"gametype\":\"%s\",", json_escape(server->gametype));
	printf("\"map\":\"%s\",", json_escape(server->map));

	printf("\"lastseen\":\"%s\",", json_date(server->lastseen));
	printf("\"expire\":\"%s\",", json_date(server->expire));

	printf("\"num_clients\":%d,", server->num_clients);
	printf("\"max_clients\":%d,", server->max_clients);

	printf("\"clients\":[");

	foreach_server_client(query, &c, "ss", server->ip, server->port) {
		if (nrow)
			putchar(',');

		putchar('{');
		printf("\"name\":\"%s\",", json_hexstring(c.name));
		printf("\"clan\":\"%s\",", json_hexstring(c.clan));
		printf("\"score\":%d,", c.score);
		printf("\"ingame\":%s", json_boolean(c.ingame));
		putchar('}');
	}
	putchar(']');

	putchar('}');
}

int main_json_server(struct url *url)
{
	char *addr = NULL, *ip, *port;
	struct server server;
	sqlite3_stmt *res;
	unsigned i, nrow;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "addr") == 0)
			addr = url->args[i].val;
	}

	if (!addr || !parse_addr(addr, &ip, &port))
		return EXIT_NOT_FOUND;

	foreach_extended_server(query, &server, "ss", ip, port);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	json_server(&server);

	return EXIT_SUCCESS;
}
