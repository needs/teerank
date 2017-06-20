#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "server.h"
#include "teerank.h"

void read_server(sqlite3_stmt *res, void *s_)
{
	struct server *s = s_;

	snprintf(s->ip, sizeof(s->ip), "%s", sqlite3_column_text(res, 0));
	snprintf(s->port, sizeof(s->port), "%s", sqlite3_column_text(res, 1));
	snprintf(s->name, sizeof(s->name), "%s", sqlite3_column_text(res, 2));
	snprintf(s->gametype, sizeof(s->gametype), "%s", sqlite3_column_text(res, 3));
	snprintf(s->map, sizeof(s->map), "%s", sqlite3_column_text(res, 4));

	s->lastseen = sqlite3_column_int64(res, 5);
	s->expire = sqlite3_column_int64(res, 6);

	snprintf(s->master_node, sizeof(s->master_node), "%s", sqlite3_column_text(res, 7));
	snprintf(s->master_service, sizeof(s->master_service), "%s", sqlite3_column_text(res, 8));

	s->max_clients = sqlite3_column_int(res, 9);
	s->num_clients = 0;
}

void read_server_client(sqlite3_stmt *res, void *_c)
{
	struct client *c = _c;

	snprintf(c->name, sizeof(c->name), "%s", sqlite3_column_text(res, 0));
	snprintf(c->clan, sizeof(c->clan), "%s", sqlite3_column_text(res, 1));

	c->score = sqlite3_column_int(res, 2);
	c->ingame = sqlite3_column_int(res, 3);
}

int read_server_clients(struct server *server)
{
	unsigned nrow;
	sqlite3_stmt *res;

	const char *query =
		"SELECT" ALL_SERVER_CLIENT_COLUMNS
		" FROM server_clients"
		" WHERE ip = ? AND port = ?"
		" ORDER BY score DESC";

	foreach_server_client(query, &server->clients[nrow], "ss", server->ip, server->port)
		if (nrow == MAX_CLIENTS)
			break_foreach;

	if (!res)
		return 0;

	server->num_clients = nrow;
	return 1;
}

static int flush_server_clients(const char *ip, const char *port)
{
	const char *query =
		"DELETE FROM server_clients"
		" WHERE ip = ? AND port = ?";

	return exec(query, "ss", ip, port);
}

int write_server_clients(struct server *server)
{
	struct client *client;
	int ret = 1;

	const char *query =
		"INSERT OR REPLACE INTO server_clients"
		" VALUES (?, ?, ?, ?, ?, ?)";

	if (!flush_server_clients(server->ip, server->port))
		return 0;

	for (client = server->clients; client - server->clients < server->num_clients; client++)
		ret &= exec(query, bind_client(*server, *client));

	return ret;
}

int write_server(struct server *server)
{
	const char *query =
		"INSERT OR REPLACE INTO servers"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	return exec(query, bind_server(*server));
}

int server_expired(struct server *server)
{
	time_t now;

	assert(server != NULL);

	now = time(NULL);

	if (now == (time_t)-1)
		return 1;

	return now > server->expire;
}

static void remove_server_clients(const char *ip, const char *port)
{
	const char *query =
		"DELETE FROM server_clients"
		" WHERE ip = ? AND port = ?";

	assert(ip != NULL);
	assert(port != NULL);

	exec(query, "ss", ip, port);
}

void remove_server(const char *ip, const char *port)
{
	const char *query =
		"DELETE FROM servers"
		" WHERE ip = ? AND port = ?";

	assert(ip != NULL);
	assert(port != NULL);

	remove_server_clients(ip, port);
	exec(query, "ss", ip, port);
}

struct server create_server(
	const char *ip, const char *port,
	const char *master_node, const char *master_service)
{
	struct server s = { 0 };

	snprintf(s.ip, sizeof(s.ip), "%s", ip);
	snprintf(s.port, sizeof(s.port), "%s", port);
	snprintf(s.master_node, sizeof(s.master_node), "%s", master_node);
	snprintf(s.master_service, sizeof(s.master_service), "%s", master_service);

	const char *query =
		"INSERT OR IGNORE INTO servers(" ALL_SERVER_COLUMNS ")"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	exec(query, bind_server(s));
	return s;
}
