#ifndef DELTA_H
#define DELTA_H

#include <time.h>

#define MAX_PLAYERS 16
struct delta {
	time_t elapsed;
	unsigned length;
	struct player_delta {
		char *name, *clan;
		long delta;
		long score;
		long elo;
	} players[MAX_PLAYERS];
};

/* Read a struct delta on stdin */
int scan_delta(struct delta *delta);

/* Print delta on stdin */
void print_delta(struct delta *delta);

#endif /* DELTA_H */
