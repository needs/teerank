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
	" name, clan, elo, lastseen, server_ip, server_port "

#define RANK_COLUMN \
	" (SELECT COUNT(1) + 1" \
	"  FROM players AS p2" \
	"  WHERE players.elo < p2.elo" \
	"   OR (players.elo = p2.elo" \
	"    AND (players.lastseen < p2.lastseen" \
	"     OR (players.lastseen = p2.lastseen" \
	"      AND players.name < p2.name))))" \
	" AS rank "

#define ALL_EXTENDED_PLAYER_COLUMNS \
	ALL_PLAYER_COLUMNS "," RANK_COLUMN

#define SORT_BY_ELO \
	" elo DESC, lastseen DESC, name DESC "
#define SORT_BY_LASTSEEN \
	" lastseen DESC, elo DESC, name DESC "

#define foreach_player(query, p, ...) \
	foreach_row(query, read_player, p, __VA_ARGS__)

#define foreach_extended_player(query, p, ...) \
	foreach_row(query, read_extended_player, p, __VA_ARGS__)

void read_player(sqlite3_stmt *res, void *p);
void read_extended_player(sqlite3_stmt *res, void *p);

struct player_record {
	time_t ts;
	int elo;
	unsigned rank;
};

#define ALL_PLAYER_RECORD_COLUMNS \
	" timestamp, elo, rank "

#define foreach_player_record(query, r, ...) \
	foreach_row(query, read_player_record, r, __VA_ARGS__)

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

	struct player_delta *delta;

	/* Used to notice update-clan when clan changed */
	unsigned short clan_changed;

	/* A marker used by elo rating system */
	short is_rankable;
};

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
 * Create a player with the given name.
 *
 * The player is *not* written in the database, but it can be written
 * with write_player().
 *
 * @param player Player to be created
 * @param name Name of the new player
 */
void create_player(struct player *player, const char *name);

/**
 * Write a player to the database.
 *
 * @param player Player to write
 *
 * @return 1 on success, 0 on failure
 */
int write_player(struct player *player);

/**
 * Change current clan of the given player.
 *
 * @param player Player to update clan
 * @param rank New clan string for the given player
 */
void set_clan(struct player *player, char *clan);

/**
 * Update lastseen date and set server port and IP
 *
 * @param player Player to update lastseen date
 * @param ip IP of the server player is actually playing on
 * @param port Port of the server player is actually playing on
 */
void set_lastseen(struct player *player, const char *ip, const char *port);

/**
 * Add an entry in player historic
 *
 * @param player Player to update
 */
void record_elo_and_rank(struct player *player);

/**
 * Count the number of players in the database
 */
unsigned count_players(void);

#endif /* PLAYER_H */
