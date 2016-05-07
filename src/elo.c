#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include "elo.h"
#include "player.h"
#include "delta.h"
#include "config.h"

/* p() func as defined by Elo. */
static double p(double delta)
{
	if (delta > 400.0)
		delta = 400.0;
	else if (delta < -400.0)
		delta = -400.0;

	return 1.0 / (1.0 + pow(10.0, -delta / 400.0));
}

/* Classic Elo formula for two players */
static int compute_elo_delta(struct player *player, struct player *opponent)
{
	static const unsigned K = 25;
	double W;

	assert(player != NULL);
	assert(opponent != NULL);
	assert(player != opponent);

	if (player->delta->delta < opponent->delta->delta)
		W = 0.0;
	else if (player->delta->delta == opponent->delta->delta)
		W = 0.5;
	else
		W = 1.0;

	return K * (W - p(player->elo - opponent->elo));
}

/*
 * Elo has been designed for two players games only.  In order to have
 * meaningul value for multiplayer, we have to modify how new Elos are
 * computed.
 *
 * To get the new Elo for a player, we match this player against every
 * other players and we make the average of every Elo deltas.  The Elo
 * delta is then added to the player's Elo points.
 */
int compute_new_elo(struct player *player, struct player *players, unsigned length)
{
	unsigned i;
	int total = 0;

	assert(player != NULL);
	assert(players != NULL);
	assert(length <= MAX_PLAYERS);

	for (i = 0; i < length; i++)
		if (&players[i] != player && players[i].is_rankable)
			total += compute_elo_delta(player, &players[i]);

	total = total / (int)length;

	return player->elo + total;
}

static void print_elo_change(struct player *player, int elo)
{
	static char name[NAME_LENGTH];

	hexname_to_name(player->name, name);
	verbose("\t%-32s | %-16s | %d -> %d (%+d)\n", player->name, name,
	        player->elo, elo, elo - player->elo);
}

#include <stdio.h>

void update_elos(struct player *players, unsigned length)
{
	int elos[MAX_PLAYERS];
	unsigned i;

	assert(players != NULL);
	assert(length <= MAX_PLAYERS);

	/*
	 * Do it in two steps so that newly computed elos values do not
	 * interfer with the computing of the next ones.
	 */

	for (i = 0; i < length; i++) {
		if (players[i].is_rankable) {
			elos[i] = compute_new_elo(&players[i], players, length);
			print_elo_change(&players[i], elos[i]);
		}
	}

	for (i = 0; i < length; i++)
		if (players[i].is_rankable)
			update_elo(&players[i], elos[i]);
}
