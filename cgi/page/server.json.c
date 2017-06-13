#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "server.h"

static void json_server(struct server *server)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct client c;

	const char *query =
		"SELECT" ALL_SERVER_CLIENT_COLUMNS
		" FROM server_clients"
		" WHERE ip IS ? AND port IS ?"
		" ORDER BY" SORT_BY_SCORE;

	json("{");
	json("%s:%s,", "ip", server->ip);
	json("%s:%s,", "port", server->port);

	json("%s:%s,", "name", server->name);
	json("%s:%s,", "gametype", server->gametype);
	json("%s:%s,", "map", server->map);

	json("%s:%d,", "lastseen", server->lastseen);
	json("%s:%d,", "expire", server->expire);

	json("%s:%u,", "num_clients", server->num_clients);
	json("%s:%u,", "max_clients", server->max_clients);

	json("%s:[", "clients");

	foreach_server_client(query, &c, "ss", server->ip, server->port) {
		if (nrow)
			json(",");

		json("{");
		json("%s:%s,", "name", c.name);
		json("%s:%s,", "clan", c.clan);
		json("%s:%d,", "score", c.score);
		json("%s:%b", "ingame", c.ingame);
		json("}");
	}
	json("]");

	json("}");
}

void generate_json_server(struct url *url)
{
	struct server server;
	sqlite3_stmt *res;
	unsigned i, nrow;

	char *ip = DEFAULT_PARAM_VALUE(PARAM_IP(0));
	char *port = DEFAULT_PARAM_VALUE(PARAM_PORT(0));

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip IS ? AND port IS ?";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "ip") == 0)
			ip = url->args[i].val;
		if (strcmp(url->args[i].name, "port") == 0)
			port = url->args[i].val;
	}

	if (!ip)
		error(400, "Missing 'ip' parameter");
	if (!port)
		error(400, "Missing 'port' parameter");

	foreach_extended_server(query, &server, "ss", ip, port);
	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);

	json_server(&server);
}
