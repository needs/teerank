#ifndef ELO_H
#define ELO_H

#include "player.h"

/* Number of elo points new players start with */
static const int DEFAULT_ELO = 1500;

/*
 * Update elo's point of each rankable player.
 */
void compute_elos(struct player *players, unsigned length);

#endif /* ELO_H */
