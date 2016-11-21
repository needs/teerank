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
#include "config.h"
#include "json.h"

int is_vanilla_ctf_server(
	const char *gametype, const char *map, int num_clients, int max_clients)
{
	const char **maps = (const char*[]) {
		"ctf1", "ctf2", "ctf3", "ctf4", "ctf5", "ctf6", "ctf7", NULL
	};

	if (strcmp(gametype, "CTF") != 0)
		return 0;

	if (num_clients > MAX_CLIENTS)
		return 0;
	if (max_clients > MAX_CLIENTS)
		return 0;

	while (*maps && strcmp(map, *maps) != 0)
		maps++;

	if (!*maps)
		return 0;

	return 1;
}

void server_from_result_row(struct server *server, sqlite3_stmt *res, int read_num_clients)
{
	snprintf(server->ip, sizeof(server->ip), "%s", sqlite3_column_text(res, 0));
	snprintf(server->port, sizeof(server->port), "%s", sqlite3_column_text(res, 1));
	snprintf(server->name, sizeof(server->name), "%s", sqlite3_column_text(res, 2));
	snprintf(server->gametype, sizeof(server->gametype), "%s", sqlite3_column_text(res, 3));
	snprintf(server->map, sizeof(server->map), "%s", sqlite3_column_text(res, 4));

	server->lastseen = sqlite3_column_int64(res, 5);
	server->expire = sqlite3_column_int64(res, 6);

	snprintf(server->master_node, sizeof(server->master_node), "%s", sqlite3_column_text(res, 7));
	snprintf(server->master_service, sizeof(server->master_service), "%s", sqlite3_column_text(res, 8));

	server->max_clients = sqlite3_column_int(res, 9);

	if (read_num_clients)
		server->num_clients = sqlite3_column_int(res, 10);
}

int read_server_clients(struct server *server)
{
	int ret;
	sqlite3_stmt *res;
	struct client *client;
	const char query[] =
		"SELECT name, clan, score, ingame"
		" FROM server_clients"
		" WHERE ip = ? AND port = ?"
		" ORDER BY ingame DESC, score DESC";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 1, server->ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, server->port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	for (client = server->clients; client - server->clients < MAX_CLIENTS; client++) {
		if ((ret = sqlite3_step(res)) != SQLITE_ROW)
			break;

		snprintf(client->name, sizeof(client->name), "%s", sqlite3_column_text(res, 0));
		snprintf(client->clan, sizeof(client->clan), "%s", sqlite3_column_text(res, 1));

		client->score = sqlite3_column_int(res, 2);
		client->ingame = sqlite3_column_int(res, 3);
	}

	if (ret != SQLITE_DONE && ret != SQLITE_ROW)
		goto fail;

	server->num_clients = client - server->clients;

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: read_server_clients(%s, %s): %s\n",
	        config.dbpath, server->ip, server->port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

int read_server(struct server *server, const char *ip, const char *port)
{
	int ret;
	sqlite3_stmt *res;
	const char query[] =
		"SELECT" ALL_SERVER_COLUMN
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 1, ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	ret = sqlite3_step(res);
	if (ret == SQLITE_DONE)
		goto not_found;
	else if (ret != SQLITE_ROW)
		goto fail;

	server_from_result_row(server, res, 0);
	sqlite3_finalize(res);
	return SUCCESS;

not_found:
	sqlite3_finalize(res);
	return NOT_FOUND;
fail:
	fprintf(
		stderr, "%s: read_server(%s, %s): %s\n",
	        config.dbpath, ip, port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return FAILURE;
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
	sqlite3_stmt *res;
	const char query[] =
		"DELETE FROM server_clients"
		" WHERE ip = ? AND port = ?";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: flush_server_clients(%s, %s): %s\n",
	        config.dbpath, ip, port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

int write_server_clients(struct server *server)
{
	sqlite3_stmt *res;
	struct client *client;
	const char query[] =
		"INSERT OR REPLACE INTO server_clients"
		" VALUES (?, ?, ?, ?, ?, ?)";

	if (!flush_server_clients(server->ip, server->port))
		return 0;

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	for (client = server->clients; client - server->clients < server->num_clients; client++) {
		if (sqlite3_bind_text(res, 1, server->ip, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_text(res, 2, server->port, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_text(res, 3, client->name, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_text(res, 4, client->clan, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_int(res, 5, client->score) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_int(res, 6, client->ingame) != SQLITE_OK)
			goto fail;

		if (sqlite3_step(res) != SQLITE_DONE)
			goto fail;

		if (sqlite3_reset(res) != SQLITE_OK)
			goto fail;
		if (sqlite3_clear_bindings(res) != SQLITE_OK)
			goto fail;
	}

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: write_server_clients(%s, %s): %s\n",
	        config.dbpath, server->ip, server->port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

int write_server(struct server *server)
{
	sqlite3_stmt *res;
	const char query[] =
		"INSERT OR REPLACE INTO servers(" ALL_SERVER_COLUMN ")"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, server->ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, server->port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 3, server->name, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 4, server->gametype, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 5, server->map, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 6, server->lastseen) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int64(res, 7, server->expire) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 8, server->master_node, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 9, server->master_service, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 10, server->max_clients) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: write_server(%s, %s): %s\n",
		config.dbpath, server->ip, server->port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
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

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

void mark_server_offline(struct server *server)
{
	time_t now;

	assert(server != NULL);

	/*
	 * We won't want to check an offline server too often, because it will
	 * add a (probably) unnecessary 3 seconds delay when polling.  However
	 * sometime the server is online but our UDP packets get lost 3 times
	 * in a row, in this case we don't want to delay too much the next poll.
	 *
	 * To meet the requirements above, we schedule the next poll to:
	 *
	 * 	now + min(now - server->lastseen, 2 hours)
	 *
	 * So for example if the server was seen 5 minutes ago, the next poll
	 * will be schedule in 5 minutes.  If the server is still offline 5
	 * minutes later, then we schedule the next poll in 10 minutes...  Up
	 * to a maximum of 2 hours.
	 */

	now = time(NULL);
	server->expire = now + min(now - server->lastseen, 2 * 3600);
}

/*
 * An interesting server is a server that will be polled more often than
 * the others.  In our case we want to poll regularly vanilla CTF
 * servers because we only rank players of such servers.
 */
static int is_interesting_server(struct server *server)
{
	return is_vanilla_ctf_server(
		server->gametype, server->map,
		server->num_clients, server->max_clients);
}

void mark_server_online(struct server *server)
{
	time_t now;
	static int initsrand = 1;

	assert(server != NULL);

	now = time(NULL);
	server->lastseen = now;

	if (is_interesting_server(server)) {
		server->expire = 0;
	} else {
		if (initsrand) {
			initsrand = 0;
			srand(now);
		}

		/*
		 * We just choose a random value between a half hour and
		 * one and a half hour to spread server updates over
		 * mutliple run of update-server.
		 */
		server->expire = now + 1800 + 3600 * ((double)rand() / (double)RAND_MAX);
	}
}

static void remove_server_clients(const char *ip, const char *port)
{
	sqlite3_stmt *res;
	const char query[] =
		"DELETE FROM server_clients"
		" WHERE ip = ? AND port = ?";

	assert(ip != NULL);
	assert(port != NULL);

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return;

fail:
	fprintf(
		stderr, "%s: remove_server_clients(%s, %s): %s\n",
		config.dbpath, ip, port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
}

void remove_server(const char *ip, const char *port)
{
	sqlite3_stmt *res;
	const char query[] =
		"DELETE FROM servers"
		" WHERE ip = ? AND port = ?";

	assert(ip != NULL);
	assert(port != NULL);

	remove_server_clients(ip, port);

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return;

fail:
	fprintf(
		stderr, "%s: remove_server(%s, %s): %s\n",
		config.dbpath, ip, port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
}

int create_server(
	const char *ip, const char *port,
	const char *master_node, const char *master_service)
{
	sqlite3_stmt *res;
	const char query[] =
		"INSERT OR IGNORE INTO servers(" ALL_SERVER_COLUMN ")"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	if (sqlite3_bind_text(res, 1, ip, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 2, port, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_null(res, 3) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_null(res, 4) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_null(res, 5) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 6, NEVER_SEEN) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 7, 0) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 8, master_node, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_text(res, 9, master_service, -1, SQLITE_STATIC) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_null(res, 10) != SQLITE_OK)
		goto fail;

	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: create_server(%s, %s): %s\n",
		config.dbpath, ip, port, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

unsigned count_vanilla_servers(void)
{
	unsigned retval;
	struct sqlite3_stmt *res;
	char query[] =
		"SELECT COUNT(1) FROM servers WHERE" IS_VANILLA_CTF_SERVER;

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_step(res) != SQLITE_ROW)
		goto fail;

	retval = sqlite3_column_int64(res, 0);

	sqlite3_finalize(res);
	return retval;

fail:
	fprintf(
		stderr, "%s: count_servers(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}
