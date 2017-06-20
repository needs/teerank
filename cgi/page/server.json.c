#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "json.h"

struct server {
	char *ip;
	char *port;
	char *name;
	char *gametype;
	char *map;

	time_t lastseen;
	time_t expire;
	unsigned max_clients;
};

static void read_server(sqlite3_stmt *res, void *s_)
{
	struct server *s = s_;

	s->ip = (char *)sqlite3_column_text(res, 0);
	s->port = (char *)sqlite3_column_text(res, 1);
	s->name = (char *)sqlite3_column_text(res, 2);
	s->gametype = (char *)sqlite3_column_text(res, 3);
	s->map = (char *)sqlite3_column_text(res, 4);

	s->lastseen = sqlite3_column_int64(res, 5);
	s->expire = sqlite3_column_int64(res, 6);
	s->max_clients = sqlite3_column_int(res, 7);
}

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

	json("%s:%u,", "max_clients", server->max_clients);

	json("%s:", "clients");

	res = foreach_init(
		query, "ssss",
		server->gametype, server->map,
		server->ip, server->port);
	json_list(res, cols, "num_clients");

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
		"SELECT ip, port, name, gametype, map, lastseen, expire, max_clients"
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

	foreach_row(query, read_server, &server, "ss", ip, port);
	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);

	json_server(&server);
}
