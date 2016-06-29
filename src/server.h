#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "player.h"

#define MAX_CLIENTS 16

struct server_state {
	time_t last_seen;
	time_t expire;

	char *gametype;
	char *map;

	int num_clients;
	int max_clients;
	struct client {
		char name[HEXNAME_LENGTH], clan[HEXNAME_LENGTH];
		long score;
		long ingame;
	} clients[MAX_CLIENTS];
};

int read_server_state(struct server_state *state, char *server_name);
int write_server_state(struct server_state *state, const char *server_name);

void mark_server_offline(struct server_state *state);
void mark_server_online(struct server_state *state, int expire_now);

int server_expired(struct server_state *state);

/*
 * Just delete file associated to the given server name
 */
void remove_server(const char *sname);

/*
 * Create an empty server file, use server_exist() if you don't want
 * to overide an already existing server.
 */
int create_server(const char *sname);

/*
 * Check wether or not a server exist.
 */
int server_exist(const char *sname);

#endif /* SERVER_H */
