#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "server.h"

int read_server_state(struct server_state *state, FILE *file, char *path)
{
	unsigned i;
	int ret;

	assert(state != NULL);
	assert(file != NULL);
	assert(path != NULL);

	ret = fscanf(file, "%d", &state->num_clients);

	if (ferror(file)) {
		perror(path);
		return 0;
	} else if (ret == EOF) {
		state->gametype = NULL;
		return 1;
	} else if (ret == 0) {
		fprintf(stderr, "%s: Cannot match clients number\n", path);
		return 0;
	}

	state->gametype = "CTF";
	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		ret = fscanf(file, " %s %s %ld",
		             client->name, client->clan, &client->score);
		if (ferror(file)) {
			perror(path);
			return 0;
		} else if (ret == EOF) {
			fprintf(stderr, "%s: Early end-of-file\n", path);
			return 0;
		} else if (ret < 3) {
			fprintf(stderr, "%s: Only %d elements matched\n",
			        path, ret);
			return 0;
		}
	}

	return 1;
}

int write_server_state(struct server_state *state, FILE *file, char *path)
{
	unsigned i;

	assert(state != NULL);
	assert(file != NULL);
	assert(path != NULL);

	if (state->num_clients == 0)
		return 1;
	if (fprintf(file, "%d\n", state->num_clients) <= 0)
		return perror(path), 0;

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		if (fprintf(file, "%s %s %ld\n",
		            client->name, client->clan, client->score) <= 0)
			return perror(path), 0;
	}

	return 1;
}
