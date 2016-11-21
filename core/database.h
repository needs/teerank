#ifndef DATABASE_H
#define DATABASE_H

#include <stdarg.h>
#include <sqlite3.h>

void foreach_init(sqlite3_stmt **res, const char *query, const char *bindfmt, ...);
int  foreach_next(sqlite3_stmt **res, void *data, void (*read_row)(sqlite3_stmt*, void*));

#define foreach_row(query, read_row, m, ...) for ( \
	foreach_init(&res, query, "" __VA_ARGS__), nrow = 0; \
	foreach_next(&res, m, read_row);           nrow++ \
)

#define foreach_break { sqlite3_finalize(res); break; }

#endif /* DATABASE_H */
