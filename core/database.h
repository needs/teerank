#ifndef DATABASE_H
#define DATABASE_H

#include <stdarg.h>
#include <sqlite3.h>

extern sqlite3 *db;
int init_database(int readonly);

/*
 * Query database version.  It does require database handle to be
 * opened, as it will query version from the "version" table.
 */
int database_version(void);

/* Last time the database was updated */
time_t last_database_update(void);

/*
 * Helper to return the result of query counting rows.  hence queries
 * should look like: "SELECT COUNT(1) FROM ...".
 */
#define count_rows(query, ...) _count_rows(query, "" __VA_ARGS__)
unsigned _count_rows(const char *query, const char *bindfmt, ...);

/*
 * Helper to execute a query yielding no results, and binds data before
 * executing the query.  exec() keep the last query in memory, hence
 * reusing a query should not prepare() it each time.
 */
#define exec(query, ...) _exec(query, "" __VA_ARGS__)
int _exec(const char *query, const char *bindfmt, ...);

/*
 * Helper to read result row from queries.  Prepare, run and clean up
 * resources necessary to process the given query.  User must have
 * declared "sqlite3_stmt *res; unsigned nrow;".
 */
#define foreach_row(query, read_row, buf, ...) for ( \
	res = foreach_init(query, "" __VA_ARGS__), nrow = 0; \
	foreach_next(&res, (buf), read_row);       nrow++ \
)
sqlite3_stmt *foreach_init(const char *query, const char *bindfmt, ...);
int foreach_next(sqlite3_stmt **res, void *data, void (*read_row)(sqlite3_stmt*, void*));

/* Should be used instead of break; to exit foreach_row() loop */
#define break_foreach { sqlite3_finalize(res); break; }

/*
 * The following functions are used when doing bulk insert/update.
 * Droping index in such cases helps to increase performances because
 * indices are not re-calculated at each insert/update.
 */
void create_all_indices(void);
void drop_all_indices(void);

#endif /* DATABASE_H */
