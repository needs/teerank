#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>
#include <time.h>

#include "network.h"
#include "historic.h"

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

	struct historic elo_historic;
	struct historic hourly_rank;
	struct historic daily_rank;
	struct historic monthly_rank;

	struct player_delta *delta;

	/* Keep tracks of changed fields to avoid unecessary write_player() */
	unsigned short is_modified;

	/* A marker used by elo rating system */
	short is_rankable;
};

/**
 * @def INVALID_ELO
 *
 * Value used to mark the absence of ELO points.
 */
static const int      INVALID_ELO  = INT_MIN;

/**
 * @def INVALID_RANK
 *
 * Value used to mark the absence of any rank.
 */
static const unsigned INVALID_RANK = 0;

enum {
	IS_MODIFIED_CREATED = (1 << 0),
	IS_MODIFIED_CLAN    = (1 << 1),
	IS_MODIFIED_ELO     = (1 << 2),
	IS_MODIFIED_RANK    = (1 << 3)
};

/**
 * @def PLAYER_ZERO
 *
 * Players used for the first time must have been assigned to this symbolic
 * constant.
 */
const struct player PLAYER_ZERO;

/**
 * Read from the disk a player.  This function allocate or reuse buffers
 * allocated by previous calls.  Hence player must be initialized to
 * zero using PLAYER_ZERO on the first call.
 *
 * Even if read fail, the player is suitable for printing.
 *
 * @param player Player to read
 * @param name Name of the player to read
 *
 * @return 1 on success, 0 on failure
 */
int read_player(struct player *player, char *name);

/**
 * Write a player to the disk.
 *
 * @param player Player to write
 *
 * @return 1 on success, 0 on failure
 */
int write_player(struct player *player);

/**
 * Change current elo point of the given player.  Add a record to elo historic.
 * It may fail because adding a record may need a realloc().
 *
 * @param player Player to update elo
 * @param elo New elo value for the given player
 *
 * @return 1 on success, 0 on failure
 */
int set_elo(struct player *player, int elo);

/**
 * Change current rank of the given player.  Add a record to rank historic.
 * It may fail because adding a record may need a realloc().
 *
 * @param player Player to update rank
 * @param rank New rank value for the given player
 *
 * @return 1 on success, 0 on failure
 */
int set_rank(struct player *player, unsigned rank);

/**
 * Change current clan of the given player.
 *
 * @param player Player to update clan
 * @param rank New clan string for the given player
 */
void set_clan(struct player *player, char *clan);


/**
 * @struct player_array
 *
 * Define an easy ways to load, store and change a lot of players.
 */
struct player_array {
	struct player *players;
	unsigned length, buffer_length;
};

/**
 * Add a player to the given player array.  It copy the content of the given
 * player and then return a pointer to the copy.
 *
 * @param array Array to add the player in
 * @param player Player to be added
 *
 * @return Added player on success, NULL on failure
 */
struct player *add_player(struct player_array *array, struct player *player);


/**
 * @struct player_summary
 *
 * Hold a summary of player data.  It is lighter than player data structure
 * because it doesn't store the complete historic.  When dealing with this data
 * structure no malloc() are performed, at all.
 *
 * However, because the structure only holds a part of the complete set of
 * player's data, the structure cannot be written back on the disk.
 */
struct player_summary {
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];

	int elo;
	unsigned rank;

	struct historic_summary elo_hs;
	struct historic_summary hourly_rank;
	struct historic_summary daily_rank;
	struct historic_summary monthly_rank;
};

/**
 * Read and fill player summary.
 *
 * @param ps Player summary to be read
 * @param name Name of the player to read
 */
int read_player_summary(struct player_summary *ps, char *name);

#endif /* PLAYER_H */
