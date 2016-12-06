#ifndef ELO_H
#define ELO_H

#include "player.h"

/* When the player is new one the server, he doesn't have an old score */
#define NO_SCORE INT_MIN

struct delta {
	char ip[IP_STRSIZE];
	char port[PORT_STRSIZE];

	char gametype[GAMETYPE_STRSIZE];
	char map[MAP_STRSIZE];

	int num_clients;
	int max_clients;

	int elapsed;
	unsigned length;
	struct player_delta {
		char name[NAME_LENGTH], clan[NAME_LENGTH];
		int ingame;
		int score, old_score;
		int elo;
	} players[MAX_CLIENTS];
};

/*
 * Update elo's point of each rankable player.
 */
void compute_elos(struct player *players, unsigned length);

#endif /* ELO_H */
