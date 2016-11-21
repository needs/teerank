#ifndef MASTER_H
#define MASTER_H

#include <time.h>
#include <sqlite3.h>

#include "database.h"

#define MAX_MASTERS 8

#define ALL_MASTER_COLUMNS \
	" node, service, lastseen "

#define NSERVERS_COLUMN \
	" (SELECT COUNT(1)" \
	"  FROM servers" \
	"  WHERE master_node = node" \
	"  AND master_service = service)" \
	" AS nservers "

#define ALL_EXTENDED_MASTER_COLUMNS \
	ALL_MASTER_COLUMNS "," NSERVERS_COLUMN

struct master {
	char node[64];
	char service[8];

	time_t lastseen;
	unsigned nservers;
};

extern const struct master DEFAULT_MASTERS[5];

void read_master(sqlite3_stmt *res, void *m);
void read_extended_master(sqlite3_stmt *res, void *m);

#define foreach_master(query, m, ...) \
	foreach_row(query, read_master, m, __VA_ARGS__)

#define foreach_extended_master(query, m, ...) \
	foreach_row(query, read_extended_master, m, __VA_ARGS__)

#endif /* MASTER_H */
