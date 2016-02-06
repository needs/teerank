#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include "elo.h"
#include "delta.h"

/*
 * p() func as defined by Elo.
 */
static double p(double delta)
{
	if (delta > 400.0)
		delta = 400.0;
	else if (delta < -400.0)
		delta = -400.0;

	return 1.0 / (1.0 + pow(10.0, -delta / 400.0));
}

/* Classic Elo formula for two players */
static int compute_elo_delta(struct player_delta *player, struct player_delta *opponent)
{
	static const unsigned K = 25;
	double W;

	assert(player != NULL);
	assert(opponent != NULL);
	assert(player != opponent);

	if (player->delta < opponent->delta)
		W = 0.0;
	else if (player->delta == opponent->delta)
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
int compute_new_elo(struct delta *delta, struct player_delta *player)
{
	unsigned i;
	int total = 0;

	assert(delta != NULL);
	assert(player != NULL);
	assert(delta->length > 0);

	for (i = 0; i < delta->length; i++) {
		struct player_delta *opponent = &delta->players[i];

		if (opponent != player)
			total += compute_elo_delta(player, opponent);
	}

	total = total / (int)delta->length;

	return player->elo + total;
}
