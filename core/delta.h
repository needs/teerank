#ifndef DELTA_H
#define DELTA_H

#include "player.h"
#include "server.h"

/* Maximum player per delta */
#define MAX_PLAYERS 16

/**
 * @struct delta
 *
 * A delta between two servers
 */
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

/**
 * Scan a struct delta on stdin
 *
 * @param delta Delta struct to contain scanning result
 *
 * @return 1 on success, 0 on failure
 */
int scan_delta(struct delta *delta);

/**
 * Print a struct delta on stdin
 *
 * @param delta Delta struct to print
 */
void print_delta(struct delta *delta);

/**
 * Compare the given servers and return a delta
 *
 * Delta is ready to printed with print_delta().
 *
 * @param old Old server
 * @param new New server
 * @param elapsed Elapsed time between old and new
 *
 * @return Delta from old to new
 */
struct delta delta_servers(
	struct server *old, struct server *new, int elapsed);

#endif /* DELTA_H */
