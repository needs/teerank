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
#define ADDR_STRSIZE sizeof("[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:00000")

#include "player.h"

#define ALL_SERVER_COLUMN \
	" ip, port, name, gametype, map, lastseen, expire," \
	" master_node, master_service, max_clients "

#define NUM_CLIENTS_COLUMN \
	" (SELECT COUNT(1)" \
	"  FROM server_clients AS sc" \
	"  WHERE sc.ip = servers.ip" \
	"  AND sc.port = servers.port)" \
	" AS num_clients "

#define IS_VANILLA_CTF_SERVER \
	" gametype = 'CTF'" \
	" AND map IN ('ctf1', 'ctf2', 'ctf3', 'ctf4', 'ctf5', 'ctf6', 'ctf7')" \
	" AND max_clients <= 16 "

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

	char master_node[sizeof("masterX.teeworlds.com")];
	char master_service[sizeof("00000")];

	int num_clients;
	int max_clients;
	struct client {
		char name[HEXNAME_LENGTH], clan[HEXNAME_LENGTH];
		int score;
		int ingame;
	} clients[MAX_CLIENTS];
};

/**
 * Check if the given server info describe a vanilla ctf server
 */
int is_vanilla_ctf_server(
	const char *gametype, const char *map, int num_clients, int max_clients);

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
 * @param ip Server IP
 * @param port Server port
 *
 * @return SUCCESS on success, NOT_FOUND if the server does not exist,
 *         FAILURE on failure.
 */
int read_server(struct server *server, const char *ip, const char *port);

/**
 * Copy a result row to the provided server struct
 *
 * @param server Valid buffer to store the result in
 * @param res SQlite result row
 * @param read_num_clients The row also contains the server's number of
 *                         clients at the end
 */
void server_from_result_row(struct server *server, sqlite3_stmt *res, int read_num_clients);

/**
 * Read server's clients from the database.
 *
 * The function actually need the provided server structure to contains
 * an IP and a port, and will also use it to store the results.
 *
 * @param server Pointer to a server structure were readed data are stored
 *
 * @return 1 on success, 0 on failure
 */
int read_server_clients(struct server *server);

/**
 * Write a server in the database.
 *
 * @param server Server structure to be written
 *
 * @return 1 on success, 0 on failure
 */
int write_server(struct server *server);

/**
 * Write server's clients in the database
 *
 * @param server Server structure containing clients to be written
 *
 * @return 1 on success, 0 on failure
 */
int write_server_clients(struct server *server);

/**
 * Create an empty server in the database if it doesn't already exists.
 *
 * @param ip Server IP
 * @param port Server port
 * @param ip Master server node
 * @param port Master server service
 *
 * @return 1 on success, 0 on failure or when the server already exist.
 */
int create_server(
	const char *ip, const char *port,
	const char *master_node, const char *master_service);

/**
 * Remove a server from the database.
 *
 * @param ip Server IP
 * @param port Server port
 */
void remove_server(const char *ip, const char *port);

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
