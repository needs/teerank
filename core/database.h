#ifndef DATABASE_H
#define DATABASE_H

#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <sqlite3.h>

extern sqlite3 *db;
extern int dberr;

int init_database(int readonly);

/* Describe each table columns, used when creating a table */
#include "database.def"

/*
 * Query database version.  It does require database handle to be
 * opened, as it will query version from the "version" table.
 */
int database_version(void);

/* Last time the database was updated */
time_t last_database_update(void);

/*
 * The following functions are used when doing bulk insert/update.
 * Droping index in such cases helps to increase performances because
 * indices are not re-calculated at each insert/update.
 */
void create_all_indices(void);
void drop_all_indices(void);

/*
 * Run queries yielding no results.  Cache the last query so that
 * calling exec() on the same query multiple time should be faster.
 */
#define exec(query, ...)                                                \
	exec_(query, "" __VA_ARGS__)
bool exec_(const char *query, const char *bindfmt, ...);

/*
 * Helper to loop through results. There is no way to differentiate an
 * error from a normal loop termination.  However a message will be
 * printed on stderr when an error happen.
 */
#define foreach_row(res, query, ...) for (                              \
	(res = foreach_init(query, "" __VA_ARGS__));                    \
	(res = foreach_next(res));                                      \
)
sqlite3_stmt *foreach_init(const char *query, const char *bindfmt, ...);
sqlite3_stmt *foreach_init_(const char *query, const char *bindfmt, va_list ap);
sqlite3_stmt *foreach_next(sqlite3_stmt *res);

/* Wrappers around sqlite_column*() to correctly type the returned value */
#define column_bool(res, i)                                             \
	(bool)sqlite3_column_int(res, i)
#define column_int(res, i)                                              \
	sqlite3_column_int(res, i)
#define column_unsigned(res, i)                                         \
	(unsigned)sqlite3_column_int64(res, i)
#define column_time_t(res, i)                                           \
	(time_t)sqlite3_column_int64(res, i)
#define column_text(res, i)                                             \
	(char *)sqlite3_column_text(res, i)

/* Copy the returned value of sqlite3_column_text() in the provided
 * buffer, taking care of NULL value and size boundaries.*/
char *column_text_copy(sqlite3_stmt *res, int i, char *buf, size_t size);

/* Check for NULL value in the column */
bool is_column_null(sqlite3_stmt *res, int i);

/*
 * Helper to return the result of queries counting rows.  Queries should
 * look like: "SELECT COUNT(1) FROM ...".
 */
#define count_rows(query, ...)                                          \
	count_rows_(query, "" __VA_ARGS__)
unsigned count_rows_(const char *query, const char *bindfmt, ...);
unsigned count_rows__(const char *query, const char *bindfmt, va_list ap);

#endif /* DATABASE_H */
