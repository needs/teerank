#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "player.h"

/* Maximum clients a server state can contains */
#define MAX_CLIENTS 16

/*
 * While name and gametype string size is consistently defined accross
 * Teeworlds source code, map string size is on the other hand sometime
 * 32 byte, sometime 64 byte.  We choose 64 byte because the whole file
 * should be less than a block size anyway (>= 512b).
 */
#define SERVERNAME_STRSIZE 256
#define GAMETYPE_STRSIZE 8
#define MAP_STRSIZE 64

/**
 * @struct server_state
 *
 * Contains the state of a server at the time "last_seen".
 */
struct server_state {
	char name[SERVERNAME_STRSIZE];
	char gametype[GAMETYPE_STRSIZE];
	char map[MAP_STRSIZE];

	time_t last_seen;
	time_t expire;

	int num_clients;
	int max_clients;
	struct client {
		char name[HEXNAME_LENGTH], clan[HEXNAME_LENGTH];
		long score;
		long ingame;
	} clients[MAX_CLIENTS];
};

/**
 * Read a state file from the database.
 *
 * @param state Pointer to a state structure were readed data are stored
 * @param server_name Server name
 *
 * @return SUCCESS on success, NOT_FOUND if the server does not exist,
 *         FAILURE on failure.
 */
int read_server_state(struct server_state *state, const char *sname);

/**
 * Write a state file in the database.
 *
 * @param state State structure to be written
 * @param server_name Server name
 *
 * @return 1 on success, 0 on failure
 */
int write_server_state(struct server_state *state, const char *sname);

/**
 * Create an empty server state in the database if it doesn't already
 * exists.
 *
 * @param sname Server name
 *
 * @return 1 on success, 0 on failure or when the server already exist.
 */
int create_server(const char *sname);

/**
 * Delete a server from the database.
 *
 * @param sname Server name
 */
void remove_server(const char *sname);

/**
 * Update expiration date
 *
 * @param state State to update
 */
void mark_server_offline(struct server_state *state);

/**
 * Update last-seen date and expiration date
 *
 * If expire_now is true, server will be marked as already expired,
 * ready to be checked again.
 *
 * @param state State to update
 * @param expire_now 1 to expire server now, 0 otherwise
 */
void mark_server_online(struct server_state *state, int expire_now);

/**
 * Check if the given server expired.
 *
 * @param state State to check expiry
 *
 * @return 1 is server expired, 0 otherwise
 */
int server_expired(struct server_state *state);

#endif /* SERVER_H */
