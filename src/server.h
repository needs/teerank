#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "io.h"

#define MAX_CLIENTS 16

struct server_state {
	char *gametype;

	int num_clients;
	struct client {
		char name[MAX_NAME_LENGTH], clan[MAX_NAME_LENGTH];
		long score;
		long ingame;
	} clients[MAX_CLIENTS];

	time_t time;
};

int read_server_state(struct server_state *state, FILE *file, char *path);
int write_server_state(struct server_state *state, FILE *file, char *path);

#endif /* SERVER_H */
