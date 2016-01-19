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
};

int read_server_state(struct server_state *state, char *server_name);
int write_server_state(struct server_state *state, char *server_name);

struct server_meta {
	time_t last_seen;
	time_t expire;
};

/*
 * Since metadata are not mandatory, in case of IO failure, we just default
 * to values that makes sens, and continue (but still display a message).
 */
void read_server_meta(struct server_meta *meta, char *server_name);
void write_server_meta(struct server_meta *meta, char *server_name);

enum {
	RANDOM_EXPIRE = (1 << 0),
	SERVER_ONLINE = (1 << 1),
};
void refresh_meta(struct server_meta *meta, unsigned flags);
int server_need_refresh(struct server_meta *meta);

#endif /* SERVER_H */
