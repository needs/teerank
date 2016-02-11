#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>

#include "network.h"
#include "player.h"
#include "io.h"

static const int      INVALID_ELO  = INT_MIN;
static const unsigned INVALID_RANK = 0;

struct player {
	char name[MAX_NAME_LENGTH];
	char clan[MAX_NAME_LENGTH];

	struct player_delta *delta;
	int elo;
	unsigned rank;

	/* Avoid useless write_player() if player hasn't change during execution */
	short is_modified;

	/* A marker used by elo rating system */
	short is_rankable;
};

int create_player(struct player *player, char *name);

/*
 * Even if read fail, the player struct should be suitable for
 * html_print_player().
 */
int read_player(struct player *player, char *name);
int write_player(struct player *player);

void html_print_player(struct player *player, int show_clan_link);

#endif /* PLAYER_H */
