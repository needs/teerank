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

static void read_server(sqlite3_stmt *res, struct server *s)
{
	s->ip = column_text(res, 0);
	s->port = column_text(res, 1);
	s->name = column_text(res, 2);
	s->gametype = column_text(res, 3);
	s->map = column_text(res, 4);

	s->lastseen = column_time_t(res, 5);
	s->expire = column_time_t(res, 6);
	s->max_clients = column_unsigned(res, 7);
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
	sqlite3_stmt *res;
	bool found = false;

	char *ip;
	char *port;

	const char *query =
		"SELECT ip, port, name, gametype, map, lastseen, expire, max_clients"
		" FROM servers"
		" WHERE ip IS ? AND port IS ?";

	ip = URL_EXTRACT(url, PARAM_IP(0));
	port = URL_EXTRACT(url, PARAM_PORT(0));

	if (!ip)
		error(400, "Missing 'ip' parameter");
	if (!port)
		error(400, "Missing 'port' parameter");

	foreach_row(res, query, "ss", ip, port) {
		struct server server;
		read_server(res, &server);
		json_server(&server);
		found = true;
	}

	if (!found)
		error(404, NULL);
}
