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

void read_server(sqlite3_stmt *res, struct server *s)
{
	column_text_copy(res, 0, s->ip, sizeof(s->ip));
	column_text_copy(res, 1, s->port, sizeof(s->port));
	column_text_copy(res, 2, s->name, sizeof(s->name));
	column_text_copy(res, 3, s->gametype, sizeof(s->gametype));
	column_text_copy(res, 4, s->map, sizeof(s->map));

	s->lastseen = column_time_t(res, 5);
	s->expire = column_time_t(res, 6);

	s->max_clients = column_unsigned(res, 7);
	s->num_clients = 0;
}

void read_server_client(sqlite3_stmt *res, struct client *c)
{
	column_text_copy(res, 0, c->name, sizeof(c->name));
	column_text_copy(res, 1, c->clan, sizeof(c->clan));

	c->score = column_int(res, 2);
	c->ingame = column_bool(res, 3);
}

void read_server_clients(struct server *server)
{
	unsigned i = 0;
	sqlite3_stmt *res;

	const char *query =
		"SELECT" ALL_SERVER_CLIENT_COLUMNS
		" FROM server_clients"
		" WHERE ip = ? AND port = ?"
		" ORDER BY score DESC";

	foreach_row(res, query, "ss", server->ip, server->port) {
		if (i < MAX_CLIENTS) {
			read_server_client(res, &server->clients[i]);
			i++;
		}
	}

	server->num_clients = i;
}

static int flush_server_clients(const char *ip, const char *port)
{
	const char *query =
		"DELETE FROM server_clients"
		" WHERE ip = ? AND port = ?";

	return exec(query, "ss", ip, port);
}

int write_server_clients(struct server *sv)
{
	struct client *cl;
	int ret = 1;

	const char *query =
		"INSERT OR REPLACE INTO server_clients"
		" VALUES (?, ?, ?, ?, ?, ?)";

	if (!flush_server_clients(sv->ip, sv->port))
		return 0;

	for (cl = sv->clients; cl - sv->clients < sv->num_clients; cl++) {
		ret &= exec(
			query, "ssssii",
			sv->ip, sv->port,
			cl->name, cl->clan, cl->score, cl->ingame);
	}

	return ret;
}

int write_server(struct server *s)
{
	const char *query =
		"INSERT OR REPLACE INTO servers"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

	return exec(
		query, "sssssttu",
		s->ip, s->port, s->name, s->gametype, s->map,
		s->lastseen, s->expire, s->max_clients);
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

void remove_server(const char *ip, const char *port)
{
	assert(ip != NULL);
	assert(port != NULL);

	exec(
		"DELETE FROM servers"
		" WHERE ip = ? AND port = ?",
		"ss", ip, port
	);

	exec(
		"DELETE FROM server_clients"
		" WHERE ip = ? AND port = ?",
		"ss", ip, port
	);

	exec(
		"DELETE FROM server_masters"
		" WHERE ip = ? AND port = ?",
		"ss", ip, port
	);
}

struct server create_server(
	const char *ip, const char *port,
	const char *master_node, const char *master_service)
{
	struct server s = { 0 };

	snprintf(s.ip, sizeof(s.ip), "%s", ip);
	snprintf(s.port, sizeof(s.port), "%s", port);

	exec(
		"INSERT OR IGNORE INTO servers"
		" VALUES (?, ?, NULL, NULL, NULL, 0, 0, NULL)",
		"ss", ip, port
	);

	exec(
		"INSERT OR IGNORE INTO server_masters"
		" VALUES (?, ?, ?, ?)",
		"ssss", ip, port, master_node, master_service
	);

	return s;
}

struct client *new_client(struct server *server)
{
	if (server->received_clients == MAX_CLIENTS)
		return NULL;
	else
		return &server->clients[server->received_clients++];
}

bool is_legacy_64(struct server *server)
{
	return strstr(server->gametype, "64")
		|| strstr(server->name, "64")
		|| strstr(server->gametype, "DDraceNet")
		|| strstr(server->gametype, "DDNet");
}
