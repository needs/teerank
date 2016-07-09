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

struct player_record {
	int elo;
	unsigned rank;
};

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

	struct historic hist;

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
static const int INVALID_ELO  = INT_MIN;

/**
 * @def UNRANKED
 *
 * Value used to mark the absence of any rank.
 */
static const unsigned UNRANKED = 0;

enum {
	IS_MODIFIED_CREATED = (1 << 0),
	IS_MODIFIED_CLAN    = (1 << 1),
	IS_MODIFIED_ELO     = (1 << 2),
	IS_MODIFIED_RANK    = (1 << 3)
};

/**
 * Every struct layer must be initialized once for all, before any
 * use.  The pattern is as follow: init once, use multiple times.
 *
 * @param player Player to be initialized
 */
void init_player(struct player *player);

enum read_player_ret {
	PLAYER_FOUND,
	PLAYER_NOT_FOUND,
	PLAYER_ERROR
};

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
 * Read from the disk a player.  This function allocate or reuse buffers
 * allocated by previous calls.  Hence player must been initialized
 * with init_player() before the first call to real_player().
 *
 * If anything, the returned player is still printable, as the
 * function may have read some data before failure.
 *
 * @param player Player to read
 * @param name Name of the player to read
 *
 * @return PLAYER_FOUND on success, PLAYER_NOT_FOUND when player does
 *         not exist, PLAYER_ERROR when an error occured.
 */
enum read_player_ret read_player(struct player *player, const char *name);

/**
 * Write a player to the disk.
 *
 * @param player Player to write
 *
 * @return 1 on success, 0 on failure
 */
int write_player(struct player *player);

/**
 * Change current elo point of the given player.  Add a record to
 * historic.  It may fail because adding a record may need a
 * realloc().  Rank field of the new record is set to UNRANKED.
 *
 * @param player Player to update elo
 * @param elo New elo value for the given player
 *
 * @return 1 on success, 0 on failure
 */
int set_elo(struct player *player, int elo);

/**
 * Change current rank of the given player.  If the new last record's
 * rank was UNRANKED, it does change it with the given rank.  This
 * function will never add a new record to historic, hence it cannot
 * fail.
 *
 * @param player Player to update rank
 * @param rank New rank value for the given player
 */
void set_rank(struct player *player, unsigned rank);

/**
 * Change current clan of the given player.
 *
 * @param player Player to update clan
 * @param rank New clan string for the given player
 */
void set_clan(struct player *player, char *clan);


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

	struct historic_summary hist;
};

/**
 * Read and fill player summary.
 *
 * @param ps Player summary to be read
 * @param name Name of the player to read
 *
 * @return PLAYER_FOUND on success, PLAYER_NOT_FOUND when player does
 *         not exist, PLAYER_ERROR when an error occured.
 */
enum read_player_ret read_player_summary(struct player_summary *ps, const char *name);

#endif /* PLAYER_H */
