#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "server.h"
#include "config.h"

static char *get_path(char *server_name, char *filename)
{
	static char path[PATH_MAX];

	if (snprintf(path, PATH_MAX, "%s/servers/%s/%s",
	             config.root, server_name, filename) >= PATH_MAX) {
		fprintf(stderr, "%s: Pathname to meta file too long\n",
		        server_name);
		return NULL;
	}

	return path;
}

int read_server_state(struct server_state *state, char *server_name)
{
	unsigned i;
	int ret;
	FILE *file;
	char *path;

	assert(state != NULL);
	assert(server_name != NULL);

	/* A NULL gametype signal an empty state */
	state->gametype = NULL;

	if (!(path = get_path(server_name, "state")))
		return 0;
	if (!(file = fopen(path, "r"))) {
		if (errno != ENOENT)
			perror(path);
		return 0;
	}

	errno = 0;
	ret = fscanf(file, "%d", &state->num_clients);

	if (ret == EOF && errno != 0) {
		perror(path);
		goto fail;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match clients number\n", path);
		goto fail;
	}

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		errno = 0;
		ret = fscanf(file, " %s %s %ld",
		             client->name, client->clan, &client->score);

		if (ret == EOF && errno != 0) {
			perror(path);
			goto fail;
		} else if (ret != 3) {
			fprintf(stderr, "%s: Only %d over %d clients matched\n",
			        path, ret, state->num_clients);
			goto fail;
		}
	}

	/* Assume for now that only CTF games are ranked */
	state->gametype = "CTF";

	fclose(file);
	return 1;

fail:
	fclose(file);
	return 0;
}

int write_server_state(struct server_state *state, char *server_name)
{
	unsigned i;
	char *path;
	FILE *file;

	assert(state != NULL);
	assert(file != NULL);
	assert(path != NULL);

	if (!(path = get_path(server_name, "state")))
		return 0;
	if (!(file = fopen(path, "w")))
		return perror(path), 0;

	if (fprintf(file, "%d\n", state->num_clients) <= 0) {
		perror(path);
		goto fail;
	}

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		if (fprintf(file, "%s %s %ld\n",
		            client->name, client->clan, client->score) <= 0) {
			perror(path);
			goto fail;
		}
	}

	fclose(file);
	return 1;

fail:
	/* TODO: on failure, it might be better to remove the file to avoid
	 *       corrupted state. */
	fclose(file);
	return 0;
}

void read_server_meta(struct server_meta *meta, char *server_name)
{
	static char *path;
	FILE *file;
	time_t now;
	int ret;

	assert(meta != NULL);
	assert(server_name != NULL);

	if (!(path = get_path(server_name, "meta")))
		goto fail;
	if (!(file = fopen(path, "r"))) {
		if (errno != ENOENT)
			perror(path);
		goto fail;
	}

	errno = 0;
	ret = fscanf(file, "last seen: %ju\nexpire: %ju\n",
	             &meta->last_seen, &meta->expire);

	if (ret == EOF && errno != 0) {
		perror(path);
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Can't match 'last seen' field\n", path);
	} else if (ret == 1) {
		fprintf(stderr, "%s: Can't match 'expire' field\n", path);
	} else {
		fclose(file);
		return;
	}
	fclose(file);

fail:
	/*
	 * If metadata could not be read for whatever reason, we choose
	 * to set 'last_seen' field to now, so it wouldn't not appear to be old
	 * and therefor deleted.  We also set 'expire' field to 0 to force
	 * an update (if any).
	 */
	now = time(NULL);
	meta->last_seen = now;
	meta->expire = 0;
}

void write_server_meta(struct server_meta *meta, char *server_name)
{
	static char *path;
	FILE *file;

	assert(meta != NULL);
	assert(server_name != NULL);

	if (!(path = get_path(server_name, "meta")))
		return;
	if (!(file = fopen(path, "w")))
		return perror(path);

	/*
	 * We don't really care about a failure here, because metadata
	 * are not mandatory, plus read_server_meta() is robust.
	 */
	fprintf(file, "last seen: %ju\nexpire: %ju\n",
	        meta->last_seen, meta->expire);
	fclose(file);
}

int server_need_refresh(struct server_meta *meta)
{
	time_t now;

	assert(meta != NULL);

	if ((now = time(NULL)) == (time_t)-1)
		return 1;
	if (now > meta->expire)
		return 1;
	return 0;
}

void refresh_meta(struct server_meta *meta, unsigned flags)
{
	time_t now;
	static int initialized = 0;

	assert(meta != NULL);

	now = time(NULL);
	assert(now != (time_t)-1);

	if (flags & SERVER_ONLINE)
		meta->last_seen = now;

	if ((flags & RANDOM_EXPIRE) == 0) {
		meta->expire = 0;
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
		meta->expire = now + 1800 + 3600 * ((double)rand() / (double)RAND_MAX);
	}
}
