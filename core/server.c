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

int is_vanilla(char *gametype, char *map, unsigned max_clients)
{
	const char **maps = (const char*[]) {
		"ctf1", "ctf2", "ctf3", "ctf4", "ctf5", "ctf6", "ctf7",
		"dm1", "dm2", "dm6", "dm7", "dm8", "dm9",
		NULL
	};

	const char **gametypes = (const char*[]) {
		"CTF", "DM", "TDM",
		NULL
	};

	if (max_clients > MAX_CLIENTS)
		return 0;

	while (*gametypes && strcmp(gametype, *gametypes) != 0)
		gametypes++;
	if (!*gametypes)
		return 0;

	while (*maps && strcmp(map, *maps) != 0)
		maps++;
	if (!*maps)
		return 0;

	return 1;
}

static void _read_server(sqlite3_stmt *res, struct server *s, int extended)
{
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

	if (extended)
		s->num_clients = sqlite3_column_int(res, 10);
	else
		s->num_clients = 0;
}

void read_server(sqlite3_stmt *res, void *s)
{
	_read_server(res, s, 0);
}

void read_extended_server(sqlite3_stmt *res, void *s)
{
	_read_server(res, s, 1);
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
		" ORDER BY" SORT_BY_SCORE;

	foreach_server_client(query, &server->clients[nrow], "ss", server->ip, server->port)
		if (nrow == MAX_CLIENTS)
			break_foreach;

	if (!res)
		return 0;

	server->num_clients = nrow;
	return 1;
}

/* An IPv4 address starts by either "0." or "00." or "000." */
static int is_ipv4(const char *ip)
{
	return ip[1] == '.' || ip[2] == '.' || ip[3] == '.';
}

/*
 * IPv6 must have no shortcut, only full (with all digits) ips are
 * valid.  Also there should be no extra data after.
 */
static int is_valid_ip(const char *ip)
{
	if (is_ipv4(ip)) {
		unsigned short a, b, c, d;
		char e;

		if (sscanf(ip, "%3hu.%3hu.%3hu.%3hu%c", &a, &b, &c, &d, &e) != 4)
			return 0;

		return a < 256 && b < 256 && c < 256 && d < 256;
	} else {
		const char *pattern = "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx";

		for (; *pattern && *ip; pattern++, ip++) {
			if (*pattern == 'x' && !isxdigit(*ip))
				return 0;
			else if (*pattern == ':' && *ip != ':')
				return 0;
		}

		return *ip == '\0';
	}
}

static int is_valid_port(const char *port)
{
	long ret;
	char *end;

	ret = strtol(port, &end, 10);
	return ret >= 0 && ret < 65535 && *port && !*end;
}

int parse_addr(char *addr, char **ip, char **port)
{
	assert(addr != NULL);
	assert(ip != NULL);
	assert(port != NULL);

	if (addr[0] == '[')
		*ip = strtok(addr + 1, "]");
	else
		*ip = strtok(addr, ":");

	*port = strtok(NULL, "");

	if (!*ip || !*port)
		return 0;

	if (**port == ':')
		*port = *port + 1;

	if (!is_valid_ip(*ip) || !is_valid_port(*port))
		return 0;

	return 1;
}

char *build_addr(const char *ip, const char *port)
{
	static char buf[ADDR_STRSIZE];
	int ret;

	if (is_ipv4(ip))
		ret = snprintf(buf, sizeof(buf), "%s:%s", ip, port);
	else
		ret = snprintf(buf, sizeof(buf), "[%s]:%s", ip, port);

	if (ret >= sizeof(buf))
		return NULL;

	return buf;
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

unsigned count_vanilla_servers(void)
{
	const char *query =
		"SELECT COUNT(1) FROM servers WHERE" IS_VANILLA_CTF_SERVER;

	return count_rows(query);
}
