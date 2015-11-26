#ifndef ELO_H
#define ELO_H

#include "delta.h"

/*
 * Compute new player's Elo and return it.  It assume every players in delta
 * have their Elo points set.
 */
int compute_new_elo(struct delta *delta, struct player_delta *player);

#endif /* ELO_H */
