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

int read_server_state(struct server_state *state, const char *sname)
{
	FILE *file = NULL;
	struct jfile jfile;
	char path[PATH_MAX];
	unsigned i;

	assert(state != NULL);
	assert(sname != NULL);

	if (!dbpath(path, PATH_MAX, "servers/%s", sname))
		goto fail;

	if ((file = fopen(path, "r")) == NULL) {
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	json_read_object_start(&jfile, NULL);

	json_read_string(  &jfile, "name"     , state->name,     sizeof(state->name));
	json_read_string(  &jfile, "gametype" , state->gametype, sizeof(state->gametype));
	json_read_string(  &jfile, "map"      , state->map,      sizeof(state->map));
	json_read_unsigned(&jfile, "last_seen", (unsigned*)&state->last_seen);
	json_read_unsigned(&jfile, "expire"   , (unsigned*)&state->expire);

	json_read_int(&jfile, "num_clients", &state->num_clients);
	json_read_int(&jfile, "max_clients", &state->max_clients);

	json_read_array_start(&jfile, "clients", 0);

	if (json_have_error(&jfile))
		goto fail;

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		json_read_object_start(&jfile, NULL);

		json_read_string(&jfile, "name" , client->name, sizeof(client->name));
		json_read_string(&jfile, "clan" , client->clan, sizeof(client->clan));
		json_read_int(   &jfile, "score", (int*)&client->score);

		json_read_object_end(&jfile);

		if (json_have_error(&jfile))
			goto fail;
	}

	json_read_array_end(&jfile);
	json_read_object_end(&jfile);

	if (json_have_error(&jfile))
		goto fail;

	fclose(file);
	return 1;

fail:
	if (file)
		fclose(file);
	return 0;
}

int write_server_state(struct server_state *state, const char *sname)
{
	FILE *file = NULL;
	struct jfile jfile;
	char path[PATH_MAX];

	unsigned i;

	assert(state != NULL);

	if (!dbpath(path, PATH_MAX, "servers/%s", sname))
		goto fail;

	if ((file = fopen(path, "w")) == NULL) {
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	json_write_object_start(&jfile, NULL);

	json_write_string(  &jfile, "name"     , state->name,     sizeof(state->name));
	json_write_string(  &jfile, "gametype" , state->gametype, sizeof(state->gametype));
	json_write_string(  &jfile, "map"      , state->map,      sizeof(state->map));
	json_write_unsigned(&jfile, "last_seen", state->last_seen);
	json_write_unsigned(&jfile, "expire"   , state->expire);

	json_write_unsigned(&jfile, "num_clients", state->num_clients);
	json_write_unsigned(&jfile, "max_clients", state->max_clients);

	json_write_array_start(&jfile, "clients", 0);

	if (json_have_error(&jfile))
		goto fail;

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

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

int server_expired(struct server_state *state)
{
	time_t now;

	assert(state != NULL);

	now = time(NULL);

	if (now == (time_t)-1)
		return 1;

	return now > state->expire;
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

void mark_server_offline(struct server_state *state)
{
	time_t now;

	assert(state != NULL);

	/*
	 * We won't want to check an offline server too often, because it will
	 * add a (probably) unnecessary 3 seconds delay when polling.  However
	 * sometime the server is online but our UDP packets get lost 3 times
	 * in a row, in this case we don't want to delay too much the next poll.
	 *
	 * To meet the requirements above, we schedule the next poll to:
	 *
	 * 	now + min(now - state->last_seen, 2 hours)
	 *
	 * So for example if the server was seen 5 minutes ago, the next poll
	 * will be schedule in 5 minutes.  If the server is still offline 5
	 * minutes later, then we schedule the next poll in 10 minutes...  Up
	 * to a maximum of 2 hours.
	 */

	now = time(NULL);
	state->expire = now + min(now - state->last_seen, 2 * 3600);
}

void mark_server_online(struct server_state *state, int expire_now)
{
	time_t now;
	static int initialized = 0;

	assert(state != NULL);

	now = time(NULL);
	state->last_seen = now;

	if (expire_now) {
		state->expire = 0;
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
		state->expire = now + 1800 + 3600 * ((double)rand() / (double)RAND_MAX);
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

int create_server(const char *sname)
{
	static const struct server_state SERVER_STATE_ZERO;
	struct server_state state = SERVER_STATE_ZERO;

	strcpy(state.name, "???");
	strcpy(state.gametype, "???");
	strcpy(state.map, "???");

	state.last_seen = time(NULL);

	return write_server_state(&state, sname);
}

int server_exist(const char *sname)
{
	char path[PATH_MAX];

	if (!dbpath(path, PATH_MAX, "servers/%s", sname))
		return 0;

	return access(path, F_OK) == 0;
}
