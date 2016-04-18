#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>

#include "network.h"

/*
 * Teeworlds names cannot be bigger than 16, including terminating nul byte
 * When converting names to hexadecimal, each letter now take two bytes
 * hence the maximum name length is 16 * 2 + 1.
 */
#define NAME_LENGTH 17
#define HEXNAME_LENGTH 33

int is_valid_hexname(const char *hex);

void hexname_to_name(const char *hex, char *str);
void name_to_hexname(const char *str, char *hex);

struct player {
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];

	struct player_delta *delta;
	int elo;
	unsigned rank;

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
	IS_MODIFIED_ELO     = (1 << 2)
};

/*
 * Even if read fail, the player struct should be suitable for
 * html_print_player().
 */
int read_player(struct player *player, char *name);
int write_player(struct player *player);

/* A player array provide an easy way to store players */
struct player_array {
	struct player *players;
	unsigned length, buffer_length;
};

/* Add (by copy) the given player to the array, and return it */
struct player *add_player(struct player_array *array, struct player *player);

#endif /* PLAYER_H */
