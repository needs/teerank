#ifndef PLAYER_H
#define PLAYER_H

#include <limits.h>
#include <time.h>

#include "database.h"

/**
 * @def NAME_LENGTH
 *
 * Teeworlds names cannot have more than 16 characters, including terminating
 * nul byte.  Name under this form are standard, regular string of whatever
 * encoding.
 */
#define NAME_LENGTH 17

#include "server.h"

#define ALL_PLAYER_COLUMNS \
	" players.name, clan, lastseen, server_ip, server_port, gametype, map, elo, rank "

#define RANKED_PLAYERS_TABLE \
	" players JOIN ranks ON players.name = ranks.name "

#define SORT_BY_RANK \
	" rank ASC "
#define SORT_BY_LASTSEEN \
	" lastseen DESC, rank DESC "
#define SORT_BY_ELO \
	" elo DESC, lastseen DESC, ranks.name DESC "

/* Placeholder for ranks not yet calculated */
static const unsigned UNRANKED = 0;

/* Data imported from older database didn't recorded last seen date */
static const time_t NEVER_SEEN = 0;

/*
 * Even if the "players" table only contain data up to "server_port", we
 * never want a player without his elo and rank.  So we merge the two
 * information, considering that the "players" table will always be
 * joined with the "ranks" table.
 */
struct player {
	char name[NAME_LENGTH];
	char clan[NAME_LENGTH];

	time_t lastseen;

	char server_ip[IP_STRSIZE];
	char server_port[PORT_STRSIZE];

	char gametype[GAMETYPE_STRSIZE];
	char map[MAP_STRSIZE];
	int elo;
	unsigned rank;
};

#define foreach_player(query, p, ...) \
	foreach_row((query), read_player, (p), __VA_ARGS__)

void read_player(sqlite3_stmt *res, void *p);

/* Create in the database a player with the given name and clan */
void create_player(const char *name, const char *clan);

/* Count the number of ranked players in the database */
unsigned count_ranked_players(const char *gametype, const char *map);

/* For each player we also record his rank and elo over time */
struct player_record {
	time_t ts;
	int elo;
	unsigned rank;
};

#define ALL_PLAYER_RECORD_COLUMNS \
	" ts, elo, rank "

#define foreach_player_record(query, r, ...) \
	foreach_row((query), read_player_record, (r), __VA_ARGS__)

void read_player_record(sqlite3_stmt *res, void *p);

#endif /* PLAYER_H */
