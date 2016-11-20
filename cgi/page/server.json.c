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
	int ret;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <server_addr>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_addr(argv[1], &ip, &port))
		return EXIT_NOT_FOUND;

	if ((ret = read_server(&server, ip, port)) != SUCCESS)
		return ret;
	if (!read_server_clients(&server))
		return EXIT_FAILURE;

	json_server(&server);
	return EXIT_SUCCESS;
}
