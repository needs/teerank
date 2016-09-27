#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "player.h"

/* Maximum clients connected to a server */
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

#define IP_STRSIZE sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")
#define PORT_STRSIZE sizeof("00000")

/**
 * @struct server
 *
 * Contains the state of a server at the time "lastseen".
 */
struct server {
	char ip[IP_STRSIZE];
	char port[PORT_STRSIZE];

	char name[SERVERNAME_STRSIZE];
	char gametype[GAMETYPE_STRSIZE];
	char map[MAP_STRSIZE];

	time_t lastseen;
	time_t expire;

	int num_clients;
	int max_clients;
	struct client {
		char name[HEXNAME_LENGTH], clan[HEXNAME_LENGTH];
		int score;
		int ingame;
	} clients[MAX_CLIENTS];
};

/**
 * Read a server from the database.
 *
 * @param server Pointer to a server structure were readed data are stored
 * @param server_name Server name
 *
 * @return SUCCESS on success, NOT_FOUND if the server does not exist,
 *         FAILURE on failure.
 */
int read_server(struct server *server, const char *sname);

/**
 * Write a server in the database.
 *
 * @param server Server structure to be written
 *
 * @return 1 on success, 0 on failure
 */
int write_server(struct server *server);

/**
 * Create an empty server in the database if it doesn't already exists.
 *
 * @param ip Server IP
 * @param port Server port
 *
 * @return 1 on success, 0 on failure or when the server already exist.
 */
int create_server(const char *ip, const char *port);

/**
 * Delete a server from the database.
 *
 * @param sname Server name
 */
void remove_server(const char *sname);

/**
 * Update expiration date
 *
 * @param server Server to update
 */
void mark_server_offline(struct server *server);

/**
 * Update last-seen date and expiration date
 *
 * If expire_now is true, server will be marked as already expired,
 * ready to be checked again.
 *
 * @param server Server to update
 * @param expire_now 1 to expire server now, 0 otherwise
 */
void mark_server_online(struct server *server, int expire_now);

/**
 * Check if the given server expired.
 *
 * @param server Server to check expiry
 *
 * @return 1 is server expired, 0 otherwise
 */
int server_expired(struct server *server);

#endif /* SERVER_H */
