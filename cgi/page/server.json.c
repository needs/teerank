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
	sqlite3_stmt *res;

	struct json_list_column cols[] = {
		{ "name", "%s" },
		{ "clan", "%s" },
		{ "score", "%d" },
		{ "ingame", "%b" },
		{ "rank", "%u" },
		{ "elo", "%i" },
		{ NULL }
	};

	const char *query =
		"SELECT server_clients.name, server_clients.clan, "
		"  score, ingame, rank, elo"
		" FROM server_clients LEFT OUTER JOIN ranks"
		"  ON server_clients.name = ranks.name"
		"   AND gametype IS ? AND map IS ?"
		" WHERE ip IS ? AND port IS ?"
		" ORDER BY ingame DESC, score DESC, elo DESC";

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

	json("%s:", "clients");

	res = foreach_init(
		query, "ssss",
		server->gametype, server->map,
		server->ip, server->port);
	json_list(res, cols, NULL);

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
