#ifndef ELO_H
#define ELO_H

#include "delta.h"

/*
 * Compute new player's elo and return it.  It assume every players in delta
 * have their ELO score set.
 */
int compute_new_elo(struct delta *delta, struct player_delta *player);

#endif /* ELO_H */
