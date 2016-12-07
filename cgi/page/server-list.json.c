#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "server.h"
#include "json.h"

static void json_server(struct server *server)
{
	putchar('{');
	printf("\"ip\":\"%s\",", server->ip);
	printf("\"port\":\"%s\",", server->port);
	printf("\"name\":\"%s\",", json_escape(server->name));
	printf("\"gametype\":\"%s\",", json_escape(server->gametype));
	printf("\"map\":\"%s\",", json_escape(server->map));

	printf("\"maxplayers\":%u,", server->max_clients);
	printf("\"nplayers\":%u", server->num_clients);
	putchar('}');
}

int main_json_server_list(int argc, char **argv)
{
	unsigned nrow, pnum, offset;
	sqlite3_stmt *res;
	struct server server;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE" IS_VANILLA_CTF_SERVER
		" ORDER BY num_clients DESC"
		" LIMIT 100 OFFSET ?";

	if (argc != 3) {
		fprintf(stderr, "usage: %s <pnum> by-nplayers\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	offset = (pnum - 1) * 100;

	printf("{\"servers\":[");

	foreach_extended_server(query, &server, "u", offset) {
		if (nrow)
			putchar(',');
		json_server(&server);
	}

	printf("],\"length\":%u}", nrow);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	return EXIT_SUCCESS;
}
