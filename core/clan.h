#ifndef CLAN_H
#define CLAN_H

#include "player.h"

struct clan {
	char name[HEXNAME_LENGTH];
	unsigned nmembers;
};

#define ALL_CLAN_COLUMNS " clan, COUNT(1) AS nmembers "

#define IS_VALID_CLAN \
	" clan <> '00' "

void read_clan(sqlite3_stmt *res, void *c);

#define foreach_clan(query, m, ...) \
	foreach_row(query, read_clan, m, __VA_ARGS__)

unsigned count_clans(void);

#endif /* CLAN_H */
