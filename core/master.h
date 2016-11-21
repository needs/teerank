#ifndef MASTER_H
#define MASTER_H

#include <time.h>
#include <sqlite3.h>

#define MAX_MASTERS 8

#define ALL_MASTER_COLUMNS \
	" node, service, lastseen "

#define NSERVERS_COLUMN \
	" (SELECT COUNT(1)" \
	"  FROM servers" \
	"  WHERE master_node = node" \
	"  AND master_service = service)" \
	" AS nservers "

struct master {
	char node[64];
	char service[8];

	time_t lastseen;
	unsigned nservers;
};

extern const struct master DEFAULT_MASTERS[5];

void master_from_result_row(
	struct master *m, sqlite3_stmt *res, int read_nservers);

#endif /* MASTER_H */
