#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>
#include <time.h>

#include "network.h"
#include "historic.h"

/*
 * Teeworlds names cannot be bigger than 16, including terminating nul byte
 * When converting names to hexadecimal, each letter now take two bytes
 * hence the maximum name length is 16 * 2 + 1.
 */
#define NAME_LENGTH 17
#define HEXNAME_LENGTH 33

int is_valid_hexname(const char *hex);

void hexname_to_name(const char *hex, char *name);
void name_to_hexname(const char *name, char *hex);

struct player {
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];

	int elo;
	unsigned rank;

	struct historic elo_historic;
	struct historic rank_historic;

	struct player_delta *delta;

	/* Keep tracks of changed fields to avoid unecessary write_player() */
	unsigned short is_modified;

	/* A marker used by elo rating system */
	short is_rankable;
};

static const int      INVALID_ELO  = INT_MIN;
static const unsigned INVALID_RANK = 0;

enum {
	IS_MODIFIED_CREATED = (1 << 0),
	IS_MODIFIED_CLAN    = (1 << 1),
	IS_MODIFIED_ELO     = (1 << 2),
	IS_MODIFIED_RANK    = (1 << 3)
};

const struct player PLAYER_ZERO;

/*
 * Read from the disk a player.  This function allocate or reuse buffers
 * allocated by previous calls.  Hence player must be initialized to
 * zero using PLAYER_ZERO on the first call.
 *
 * Even if read fail, the player is suitable for printing.
 *
 * If full_history is true, then the full history is read.  It allocates
 * a bit more space than necessary because we may add entries to the history
 * and we don't want to realloc() too often.
 */
int read_player(struct player *player, char *name, int full_history);

/*
 * Player can only be writed if the full history has been successfully read.
 */
int write_player(struct player *player);

/*
 * Update the current elo point of a given player, adding an entry to the
 * history if necessary.  It can fail because history may not be wide
 * enough to add another entry.
 */
int update_elo(struct player *player, int elo);

/*
 * Set player's rank, add a new record in historic.  Can fail because historic
 * may have to be reallocated.
 */
int set_rank(struct player *player, unsigned rank);

/*
 * Free buffers but do *not* reset the struct to PLAYER_ZERO.
 */
void free_player(struct player *player);

/* A player array provide an easy way to store players */
struct player_array {
	struct player *players;
	unsigned length, buffer_length;
};

/* Add (by copy) the given player to the array, and return it */
struct player *add_player(struct player_array *array, struct player *player);

#endif /* PLAYER_H */
