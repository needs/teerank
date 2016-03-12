#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>

#include "network.h"
#include "io.h"

/*
 * Teeworlds names cannot be bigger than 16, including terminating nul byte
 * When converting names to hexadecimal, each letter now take two bytes
 * hence the maximum name length is 16 * 2 + 1.
 */
#define MAX_NAME_HEX_LENGTH 33
#define MAX_NAME_STR_LENGTH 17
#define MAX_CLAN_HEX_LENGTH 33
#define MAX_CLAN_STR_LENGTH 17

static const int      INVALID_ELO  = INT_MIN;
static const unsigned INVALID_RANK = 0;

struct player {
	char name[MAX_NAME_HEX_LENGTH];
	char clan[MAX_CLAN_HEX_LENGTH];

	struct player_delta *delta;
	int elo;
	unsigned rank;

	/* Avoid useless write_player() if player hasn't change during execution */
	short is_modified;

	/* A marker used by elo rating system */
	short is_rankable;
};

/*
 * Even if read fail, the player struct should be suitable for
 * html_print_player().
 */
int read_player(struct player *player, char *name);
int write_player(struct player *player);

void html_print_player(struct player *player, int show_clan_link);

/* A player array provide an easy way to store players */
struct player_array {
	struct player *players;
	unsigned length, buffer_length;
};

/* Add (by copy) the given player to the array, and return it */
struct player *add_player(struct player_array *array, struct player *player);

#endif /* PLAYER_H */
