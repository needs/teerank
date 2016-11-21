#include "database.h"
#include "config.h"

void foreach_init(sqlite3_stmt **res, const char *query, const char *bindfmt, ...)
{
	va_list ap;
	unsigned i;
	int ret = sqlite3_prepare_v2(db, query, -1, res, NULL);

	if (ret != SQLITE_OK) {
		*res = NULL;
		return;
	}

	va_start(ap, bindfmt);

	for (i = 0; bindfmt[i]; i++) {
		switch (bindfmt[i]) {
		case 'i':
			ret = sqlite3_bind_int(*res, i+1, va_arg(ap, int));
			break;

		case 'u':
			ret = sqlite3_bind_int64(*res, i+1, va_arg(ap, int));
			break;

		case 's':
			ret = sqlite3_bind_text(*res, i+1, va_arg(ap, char*), -1, SQLITE_STATIC);
			break;
		}

		if (ret != SQLITE_OK) {
			va_end(ap);
			sqlite3_finalize(*res);
			*res = NULL;
			return;
		}
	}

	va_end(ap);
}

int foreach_next(sqlite3_stmt **res, void *data, void (*read_row)(sqlite3_stmt*, void*))
{
	int ret = sqlite3_step(*res);

	if (!*res)
		return 0;

	if (ret == SQLITE_DONE) {
		sqlite3_finalize(*res);
		return 0;
	} else if (ret != SQLITE_ROW) {
		*res = NULL;
		sqlite3_finalize(*res);
		return 0;
	} else {
		read_row(*res, data);
		return 1;
	}
}
