#ifndef RANK_H
#define RANK_H

#include "server.h"

/*
 * Given two server state, rank players, write the updated player in the
 * database.  This is Teerank's core functionality.
 */
void rank_players(struct server *old, struct server *new);

#endif /* RANK_H */
