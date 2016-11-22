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

/**
 * @def HEXNAME_LENGTH
 *
 * Hex string represent a regular string on a different form, with only
 * hexadecimal characters (0-9, a-f).  A pair of two following characters
 * define a byte of the represented string.
 *
 * They are used to convert any string to a valid filename.  In our case only
 * teeworlds name will be converted to hex string, hence a hex string is twice
 * as long as a regular string, plus one extra byte for the terminating nul byte.
 * Hence the value 33 = (16 * 2) + 1 .
 */
#define HEXNAME_LENGTH 33

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
	" elo DESC, lastseen DESC, name "
#define SORT_BY_LASTSEEN \
	" lastseen DESC, elo DESC, name "

#define foreach_player(query, m, ...) \
	foreach_row(query, read_player, m, __VA_ARGS__)

#define foreach_extended_player(query, m, ...) \
	foreach_row(query, read_extended_player, m, __VA_ARGS__)

void read_player(sqlite3_stmt *res, void *p);
void read_extended_player(sqlite3_stmt *res, void *p);

/**
 * Check wether or not the supplied string is a valid hexadecimal string
 *
 * @param hex Hexadecimal string to be checked
 *
 * @return 1 if hex is a valid hex string, 0 else
 */
int is_valid_hexname(const char *hex);

/**
 * Convert a hex string to a regular string.  Supplied output buffer must
 * be wide enough to hold the result of the conversion.
 *
 * @param hex Hex string to be converted
 * @param name Buffer to store result of conversion
 */
void hexname_to_name(const char *hex, char *name);

/**
 * Convert a regular string to a hex string.  Supplied output buffer must
 * be wide enough to hold the result of the conversion.
 *
 * @param name Regular string to be converted
 * @param hex Buffer to store result of conversion
 */
void name_to_hexname(const char *name, char *hex);

/**
 * @struct player
 *
 * Holds a complete set of data of a player.
 */
struct player {
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];

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
