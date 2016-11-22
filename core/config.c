#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>

#include "config.h"
#include "database.h"
#include "player.h"
#include "master.h"

struct config config = {
#define STRING(envname, value, fname) \
	.fname = value,
#define BOOL(envname, value, fname) \
	.fname = value,
#define UNSIGNED(envname, value, fname) \
	.fname = value,
#include "default_config.h"
};

void load_config(int check_version)
{
	char *tmp;

#define STRING(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = tmp;
#define BOOL(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = 1;
#define UNSIGNED(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = strtol(tmp, NULL, 10);

#include "default_config.h"

	/*
	 * Update delay too short makes the ranking innacurate and
	 * volatile, and when it's too long it adds too much
	 * uncertainties.  Between 1 and 20 seems to be reasonable.
	 */
	if (config.update_delay < 1) {
		fprintf(stderr, "%u: Update delay too short, ranking will be volatile\n",
		        config.update_delay);
		exit(EXIT_FAILURE);
	} else if (config.update_delay > 20) {
		fprintf(stderr, "%u: Update delay too long, data will be uncorrelated\n",
		        config.update_delay);
		exit(EXIT_FAILURE);
	}

	/* We work with UTC time only */
	setenv("TZ", "", 1);
	tzset();

	/* Open database now so we can check it's version */
	if (sqlite3_open_v2(config.dbpath, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK)
		if (!create_database())
			exit(EXIT_FAILURE);

	/*
	 * Wait when two or more processes want to write the database.
	 * That is useful to avoid "database is locked", to some
	 * extents.  It could still happen if somehow the process has to
	 * wait more then the given timeout.
	 */
	sqlite3_busy_timeout(db, 5000);

	if (check_version) {
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
}

void verbose(const char *fmt, ...)
{
	if (config.verbose) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}

time_t last_database_update(void)
{
	struct stat st;

	if (stat(config.dbpath, &st) == -1)
		return 0;

	return st.st_mtim.tv_sec;
}
