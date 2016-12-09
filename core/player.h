#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>
#include <time.h>

#include "database.h"

/**
 * @def NAME_LENGTH
 *
 * Teeworlds names cannot have more than 16 characters, including terminating
 * nul byte.  Name under this form are standard, regular string of whatever
 * encoding.
 */
#define NAME_LENGTH 17

#include "server.h"

#define ALL_PLAYER_COLUMNS \
	" name, clan, elo, rank, lastseen, server_ip, server_port "

#define IS_PLAYER_RANKED \
	" rank > 0 "

#define SORT_BY_RANK \
	" rank ASC "
#define SORT_BY_ELO \
	" elo DESC, lastseen DESC, name DESC "
#define SORT_BY_LASTSEEN \
	" lastseen DESC, elo DESC, name DESC "

#define foreach_player(query, p, ...) \
	foreach_row((query), read_player, (p), __VA_ARGS__)

void read_player(sqlite3_stmt *res, void *p);

struct player_record {
	time_t ts;
	int elo;
	unsigned rank;
};

#define ALL_PLAYER_RECORD_COLUMNS \
	" timestamp, elo, rank "

#define foreach_player_record(query, r, ...) \
	foreach_row((query), read_player_record, (r), __VA_ARGS__)

void read_player_record(sqlite3_stmt *res, void *p);

/**
 * @struct player
 *
 * Holds a complete set of data of a player.
 */
struct player {
	char name[NAME_LENGTH];
	char clan[NAME_LENGTH];

	int elo;
	unsigned rank;
	time_t lastseen;

	char server_ip[IP_STRSIZE];
	char server_port[PORT_STRSIZE];

	/* Used by the ranking system */
	struct client *old;
	struct client *new;
	int is_rankable;
};

/**
 * @def DEFAULT_ELO
 *
 * Number of elo points new players starts with
 */
static const int DEFAULT_ELO = 1500;

/**
 * @def INVALID_ELO
 *
 * Value used to mark the absence of ELO points.
 */
static const int INVALID_ELO = INT_MIN;

/**
 * @def UNRANKED
 *
 * Value used to mark the absence of any rank.
 */
static const unsigned UNRANKED = 0;

/**
 * @def NEVER_SEEN
 *
 * Special time_t value used for player which has neveer been seen yet
 */
static const time_t NEVER_SEEN = 0;

/**
 * @def NEVER_SEEN
 *
 * Special time_t value used for player which has neveer been seen yet
 */
static const time_t EXPIRE_NOW = 0;

/**
 * Create a player with the given name.
 *
 * The player *is* written in the database.
 *
 * @param name Name of the new player
 */
void create_player(const char *name, const char *clan);

/**
 * Write a player to the database.
 *
 * @param player Player to write
 *
 * @return 1 on success, 0 on failure
 */
int write_player(struct player *player);

/**
 * Add an entry in player historic
 *
 * @param player Player to update
 */
void record_elo_and_rank(struct player *player);

/**
 * Count the number of ranked players in the database
 */
unsigned count_ranked_players(void);

#endif /* PLAYER_H */
