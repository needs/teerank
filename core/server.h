#ifndef SERVER_H
#define SERVER_H

#include <time.h>

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

#include "player.h"

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
 * Check if the given server infos describe a vanilla ctf server
 */
int is_vanilla_ctf_server(
	const char *gametype, const char *map, int num_clients, int max_clients);

/**
 * From an IP and a port, return a filename
 *
 * It returns a statically allocated buffer suitable for read_server().
 * However it does not check if it actually match any file at all.  It
 * also does not validate IP and port, so if they contain malicious
 * filepath like ".." or "/", it may cause problems either.
 *
 * @param ip Server's IP
 * @param port Server's port
 *
 * @return A statically allocated buffer with a filename
 */
char *server_filename(const char *ip, const char *port);

/**
 * Extract IP and port from a full address
 *
 * The given buffer is modified.  IP and port are checked for validity.
 * An invalid IP or port will result in a failure.
 *
 * @param addr Server address
 * @param ip extracted IP
 * @param port extracted port
 * @return 1 on success, 0 on failure
 */
int parse_addr(char *addr, char **ip, char **port);

/**
 * Construct an adress with the given ip and port
 *
 * Adress can be parsed back with parse_addr().  It returns a statically
 * allocated buffer.
 *
 * @param ip Server IP
 * @param port Server port
 * @return A statically allocated string
 */
char *build_addr(const char *ip, const char *port);

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
 * @param server Server to update
 */
void mark_server_online(struct server *server);

/**
 * Check if the given server expired.
 *
 * @param server Server to check expiry
 *
 * @return 1 is server expired, 0 otherwise
 */
int server_expired(struct server *server);

#endif /* SERVER_H */
