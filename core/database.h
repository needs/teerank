#ifndef DATABASE_H
#define DATABASE_H

#include <stdarg.h>
#include <sqlite3.h>

extern sqlite3 *db;
int create_database(void);

/*
 * Query database version.  It does require database handle to be
 * opened, as it will query version from the "version" table.
 */
int database_version(void);

/*
 * Helper to return the result of query counting rows.  hence queries
 * should look like: "SELECT COUNT(1) FROM ...".
 */
unsigned count_rows(const char *query);

/*
 * Helper to read result row from queries.  Prepare, run and clean up
 * resources necesarry to process the given query.
 */
#define foreach_row(query, read_row, buf, ...) for ( \
	foreach_init(&res, query, "" __VA_ARGS__), nrow = 0; \
	foreach_next(&res, buf, read_row);         nrow++ \
)
void foreach_init(sqlite3_stmt **res, const char *query, const char *bindfmt, ...);
int  foreach_next(sqlite3_stmt **res, void *data, void (*read_row)(sqlite3_stmt*, void*));

/* Should be used in place of 'break' to exit a foreach_row() loop */
#define break_foreach { sqlite3_finalize(res); break; }

#endif /* DATABASE_H */
