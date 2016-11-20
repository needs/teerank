#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "page.h"
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
	int ret;
	unsigned offset = 0;
	sqlite3_stmt *res;
	unsigned pnum;
	const char query[] =
		"SELECT" ALL_SERVER_COLUMN "," NUM_CLIENTS_COLUMN
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

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 1, offset) != SQLITE_OK)
		goto fail;

	if ((ret = sqlite3_step(res)) == SQLITE_DONE && pnum > 1) {
		sqlite3_finalize(res);
		return EXIT_NOT_FOUND;
	}

	printf("{\"servers\":[");

	while (ret == SQLITE_ROW) {
		struct server server;

		server_from_result_row(&server, res, 1);
		json_server(&server);
		offset++;

		if ((ret = sqlite3_step(res)) == SQLITE_ROW)
			putchar(',');
	}

	if (ret != SQLITE_DONE)
		goto fail;

	printf("],\"length\":%u}", offset - (pnum - 1) * 100);

	sqlite3_finalize(res);
	return EXIT_SUCCESS;

fail:
	fprintf(
		stderr, "%s: json_server_list(%s): %s\n",
		config.dbpath, argv[1], sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return EXIT_FAILURE;
}
