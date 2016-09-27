#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "server.h"
#include "config.h"
#include "json.h"

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
	json_read_unsigned(&jfile, "lastseen",  (unsigned*)&server->lastseen);
	json_read_unsigned(&jfile, "expire",    (unsigned*)&server->expire);

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

	json_write_string(  &jfile, "ip",   server->ip,   sizeof(server->ip));
	json_write_string(  &jfile, "port", server->port, sizeof(server->port));

	json_write_string(  &jfile, "name"    , server->name,     sizeof(server->name));
	json_write_string(  &jfile, "gametype", server->gametype, sizeof(server->gametype));
	json_write_string(  &jfile, "map"     , server->map,      sizeof(server->map));
	json_write_unsigned(&jfile, "lastseen", server->lastseen);
	json_write_unsigned(&jfile, "expire"  , server->expire);

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

void mark_server_online(struct server *server, int expire_now)
{
	time_t now;
	static int initialized = 0;

	assert(server != NULL);

	now = time(NULL);
	server->lastseen = now;

	if (expire_now) {
		server->expire = 0;
	} else {
		/*
		 * We just choose a random value between a half hour and
		 * one and a half hour, so that we do not have too much
		 * servers to update at the same time.
		 */
		if (!initialized) {
			initialized = 1;
			srand(now);
		}
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

	server.lastseen = time(NULL);

	return write_server(&server);
}
