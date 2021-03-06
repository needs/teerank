#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include "teerank.h"
#include "database.h"
#include "master.h"

/* Maximum clients connected to a server */
#define MAX_CLIENTS 64

#define ALL_SERVER_COLUMNS \
	" ip, port, name, gametype, map, lastseen, expire, max_clients "

#define NUM_CLIENTS_COLUMN \
	" (SELECT COUNT(1)" \
	"  FROM server_clients AS sc" \
	"  WHERE sc.ip = servers.ip" \
	"  AND sc.port = servers.port)" \
	" AS num_clients "

#define ALL_SERVER_CLIENT_COLUMNS \
	" name, clan, score, ingame "

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

	int received_clients;
	struct client {
		char name[NAME_STRSIZE];
		char clan[CLAN_STRSIZE];
		int score;
		int ingame;
	} clients[MAX_CLIENTS];
};

void read_server(sqlite3_stmt *res, struct server *s);

#define bind_client(s, c) "ssssii", \
	(s).ip, (s).port, (c).name, (c).clan, (c).score, (c).ingame

void read_server_client(sqlite3_stmt *res, struct client *c);

/**
 * Read server's clients from the database.
 *
 * The function actually need the provided server structure to contains
 * an IP and a port, and will also use it to store the results.
 *
 * @param server Pointer to a server structure were readed data are stored
 */
void read_server_clients(struct server *server);

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
 * @return A server struct filled with the given values
 */
struct server create_server(
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
 * Check if the given server expired.
 *
 * @param server Server to check expiry
 *
 * @return 1 is server expired, 0 otherwise
 */
int server_expired(struct server *server);

/**
 * Return the next free client slot.
 *
 * @param server Server to search for free slots
 *
 * @return A pointer to a free slot if any, NULL otherwise
 */
struct client *new_client(struct server *server);

/**
 * Returns true if the given server can handle 64 players.
 */
bool is_legacy_64(struct server *server);

#endif /* SERVER_H */
