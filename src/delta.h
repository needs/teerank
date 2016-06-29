#ifndef DELTA_H
#define DELTA_H

#include "player.h"
#include "server.h"

#define MAX_PLAYERS 16
struct delta {
	int elapsed;
	unsigned length;
	struct player_delta {
		char name[HEXNAME_LENGTH], clan[HEXNAME_LENGTH];
		long delta;
		long score;
		int elo;
	} players[MAX_PLAYERS];
};

/* Read a struct delta on stdin */
int scan_delta(struct delta *delta);

/* Print delta on stdin */
void print_delta(struct delta *delta);

struct delta delta_states(
	struct server_state *old, struct server_state *new, int elapsed);

#endif /* DELTA_H */
