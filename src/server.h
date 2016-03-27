#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "io.h"
#include "player.h"

#define MAX_CLIENTS 16

struct server_state {
	char *gametype;

	int num_clients;
	struct client {
		char name[MAX_NAME_HEX_LENGTH], clan[MAX_CLAN_HEX_LENGTH];
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

void mark_server_offline(struct server_meta *meta);
void mark_server_online(struct server_meta *meta, int expire_now);

int server_need_refresh(struct server_meta *meta);

/*
 * Delete the meta and state file, and then try to remove the directory.
 */
void remove_server(char *name);

#endif /* SERVER_H */
