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

int read_server(struct server *server, const char *sname)
{
	FILE *file = NULL;
	struct jfile jfile;
	char path[PATH_MAX];
	unsigned i;

	assert(server != NULL);
	assert(sname != NULL);

	if (!dbpath(path, PATH_MAX, "servers/%s", sname))
		goto fail;

	if ((file = fopen(path, "r")) == NULL) {
		if (errno == ENOENT)
			return NOT_FOUND;
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	json_read_object_start(&jfile, NULL);

	json_read_string(  &jfile, "ip",   server->ip,   sizeof(server->ip));
	json_read_string(  &jfile, "port", server->port, sizeof(server->port));

	json_read_string(  &jfile, "name",      server->name,     sizeof(server->name));
	json_read_string(  &jfile, "gametype",  server->gametype, sizeof(server->gametype));
	json_read_string(  &jfile, "map",       server->map,      sizeof(server->map));
	json_read_time(&jfile, "lastseen", &server->lastseen);
	json_read_time(&jfile, "expire",   &server->expire);

	json_read_int(&jfile, "num_clients", &server->num_clients);
	json_read_int(&jfile, "max_clients", &server->max_clients);

	json_read_array_start(&jfile, "clients", 0);

	if (json_have_error(&jfile))
		goto fail;

	for (i = 0; i < server->num_clients; i++) {
		struct client *client = &server->clients[i];

		json_read_object_start(&jfile, NULL);

		json_read_string(&jfile, "name" , client->name, sizeof(client->name));
		json_read_string(&jfile, "clan" , client->clan, sizeof(client->clan));
		json_read_int(   &jfile, "score", &client->score);
		json_read_bool(  &jfile, "ingame", &client->ingame);

		json_read_object_end(&jfile);

		if (json_have_error(&jfile))
			goto fail;
	}

	json_read_array_end(&jfile);
	json_read_object_end(&jfile);

	if (json_have_error(&jfile))
		goto fail;

	fclose(file);
	return SUCCESS;

fail:
	if (file)
		fclose(file);
	return FAILURE;
}

#define SERVER_FILENAME_STRSIZE \
	sizeof("vX xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx 00000")

char *server_filename(const char *ip, const char *port)
{
	static char buf[SERVER_FILENAME_STRSIZE];
	char ipv, *c;
	int ret;

	assert(ip != NULL);
	assert(port != NULL);

	if (ip[1] == '.' || ip[2] == '.' ||ip[3] == '.')
		ipv = '4';
	else
		ipv = '6';

	ret = snprintf(buf, sizeof(buf), "v%c %s %s", ipv, ip, port);
	assert(ret < sizeof(buf)); (void)ret;

	/* Replace '.' or ':' by '_' */
	for (c = buf; *c; c++)
		if (*c == '.' || *c == ':')
			*c = '_';

	return buf;
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

#define ADDR_STRSIZE sizeof("[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:00000")

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

int write_server(struct server *server)
{
	FILE *file = NULL;
	struct jfile jfile;
	char path[PATH_MAX];
	char *filename;

	unsigned i;

	assert(server != NULL);

	filename = server_filename(server->ip, server->port);
	if (!dbpath(path, PATH_MAX, "servers/%s", filename))
		goto fail;

	if ((file = fopen(path, "w")) == NULL) {
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	json_write_object_start(&jfile, NULL);

	json_write_string(&jfile, "ip",   server->ip,   sizeof(server->ip));
	json_write_string(&jfile, "port", server->port, sizeof(server->port));

	json_write_string(&jfile, "name"    , server->name,     sizeof(server->name));
	json_write_string(&jfile, "gametype", server->gametype, sizeof(server->gametype));
	json_write_string(&jfile, "map"     , server->map,      sizeof(server->map));
	json_write_time(&jfile, "lastseen", server->lastseen);
	json_write_time(&jfile, "expire"  , server->expire);

	json_write_unsigned(&jfile, "num_clients", server->num_clients);
	json_write_unsigned(&jfile, "max_clients", server->max_clients);

	json_write_array_start(&jfile, "clients", 0);

	if (json_have_error(&jfile))
		goto fail;

	for (i = 0; i < server->num_clients; i++) {
		struct client *client = &server->clients[i];

		json_write_object_start(&jfile, NULL);

		json_write_string(&jfile, "name" , client->name, sizeof(client->name));
		json_write_string(&jfile, "clan" , client->clan, sizeof(client->clan));
		json_write_int(   &jfile, "score", client->score);
		json_write_bool(  &jfile, "ingame", client->ingame);

		json_write_object_end(&jfile);

		if (json_have_error(&jfile))
			goto fail;
	}

	json_write_array_end(&jfile);
	json_write_object_end(&jfile);

	if (json_have_error(&jfile))
		goto fail;

	fclose(file);
	return 1;

fail:
	if (file)
		fclose(file);
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

void remove_server(const char *name)
{
	char path[PATH_MAX];

	assert(name != NULL);

	if (!dbpath(path, PATH_MAX, "servers/%s", name))
		return;

	if (unlink(path) == -1)
		if (errno != ENOENT)
			perror(path);
}

static int server_exist(const char *sname)
{
	char path[PATH_MAX];

	if (!dbpath(path, PATH_MAX, "servers/%s", sname))
		return 0;

	return access(path, F_OK) == 0;
}

int create_server(const char *ip, const char *port)
{
	static const struct server SERVER_ZERO;
	struct server server = SERVER_ZERO;
	char *filename;

	filename = server_filename(ip, port);
	if (server_exist(filename))
		return 0;

	if (snprintf(server.ip, sizeof(server.ip), "%s", ip) >= sizeof(server.ip)) {
		fprintf(stderr, "create_server: %s: IP too long\n", ip);
		return 0;
	}

	if (snprintf(server.port, sizeof(server.port), "%s", port) >= sizeof(server.port)) {
		fprintf(stderr, "create_server: %s: Port too long\n", port);
		return 0;
	}

	strcpy(server.name, "???");
	strcpy(server.gametype, "???");
	strcpy(server.map, "???");

	server.lastseen = NEVER_SEEN;
	server.expire = 0;

	return write_server(&server);
}
