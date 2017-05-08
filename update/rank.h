#ifndef RANK_H
#define RANK_H

#include "server.h"

/* Number of points new players start with */
static const int DEFAULT_ELO = 1500;

/*
 * Given two server state, rank players and set pending elo updates, if
 * any.  Use recompute_ranks() to actually commit those updates and
 * compute ranks as well.  This is Teerank's core functionality.
 */
void rank_players(struct server *old, struct server *new);

/*
 * Process pending elo updates and compute ranks for every players in
 * the database.
 */
void recompute_ranks(void);

#endif /* RANK_H */
