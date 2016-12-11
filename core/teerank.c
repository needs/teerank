#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "teerank.h"
#include "database.h"
#include "player.h"
#include "master.h"

struct config config = {
#define STRING(envname, value, fname) \
	.fname = value,
#define BOOL(envname, value, fname) \
	.fname = value,
#include "config.def"
};

void init_teerank(int readonly)
{
	char *tmp;

#define STRING(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = tmp;
#define BOOL(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = 1;

#include "config.def"

	/* We work with UTC time only */
	setenv("TZ", "", 1);
	tzset();

	/* Open database now so we can check it's version */
	if (!init_database(readonly))
		exit(EXIT_FAILURE);

	/*
	 * Wait when two or more processes want to write the database.
	 * That is useful to avoid "database is locked", to some
	 * extents.  It could still happen if somehow the process has to
	 * wait more then the given timeout.
	 */
	sqlite3_busy_timeout(db, 5000);

	/* Checks database version against our */
	int version = database_version();

	if (version > DATABASE_VERSION) {
		fprintf(stderr, "%s: Database too modern, upgrade your teerank installation\n",
		        config.dbpath);
		exit(EXIT_FAILURE);
	} else if (version < DATABASE_VERSION) {
		fprintf(stderr, "%s: Database outdated, upgrade it with teerank-upgrade\n",
		        config.dbpath);
		exit(EXIT_FAILURE);
	}
}

void verbose(const char *fmt, ...)
{
	if (config.verbose) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		putchar('\n');
	}
}
