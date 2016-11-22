#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "server.h"
#include "page.h"
#include "json.h"

static void json_server(struct server *server)
{
	unsigned i;

	putchar('{');
	printf("\"ip\":\"%s\",", server->ip);
	printf("\"port\":\"%s\",", server->port);

	printf("\"name\":\"%s\",", server->name);
	printf("\"gametype\":\"%s\",", server->gametype);
	printf("\"map\":\"%s\",", server->map);

	printf("\"lastseen\":\"%s\",", json_date(server->lastseen));
	printf("\"expire\":\"%s\",", json_date(server->expire));

	printf("\"num_clients\":%d,", server->num_clients);
	printf("\"max_clients\":%d,", server->max_clients);

	printf("\"clients\":[");
	for (i = 0; i < server->num_clients; i++) {
		struct client *client = &server->clients[i];

		if (i)
			putchar(',');

		putchar('{');
		printf("\"name\":\"%s\",", client->name);
		printf("\"clan\":\"%s\",", client->clan);
		printf("\"score\":%d,", client->score);
		printf("\"ingame\":%s", json_boolean(client->ingame));
		putchar('}');
	}
	putchar(']');

	putchar('}');
}

int main_json_server(int argc, char **argv)
{
	char *ip, *port;
	struct server server;
	sqlite3_stmt *res;
	unsigned nrow;

	const char query[] =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	if (argc != 2) {
		fprintf(stderr, "usage: %s <server_addr>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_addr(argv[1], &ip, &port))
		return EXIT_NOT_FOUND;

	foreach_extended_server(query, &server, "ss", ip, port);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	if (!read_server_clients(&server))
		return EXIT_FAILURE;
	json_server(&server);

	return EXIT_SUCCESS;
}
