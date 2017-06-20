#ifndef MASTER_H
#define MASTER_H

#include <time.h>
#include "database.h"

#define MAX_MASTERS 8

#define NODE_STRSIZE 64
#define SERVICE_STRSIZE 8

struct master {
	char node[NODE_STRSIZE];
	char service[SERVICE_STRSIZE];

	time_t lastseen, expire;
	unsigned nservers;
};

extern const struct master DEFAULT_MASTERS[5];

void read_master(sqlite3_stmt *res, void *m);
void read_extended_master(sqlite3_stmt *res, void *m);

int write_master(struct master *master);

#endif /* MASTER_H */
