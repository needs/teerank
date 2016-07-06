#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

#include "config.h"

struct config config = {
#define STRING(envname, value, fname) \
	.fname = value,
#define BOOL(envname, value, fname) \
	.fname = value,
#include "default_config.h"
#undef BOOL
#undef STRING
};

/*
 * Query database version.  It require config to be loaded as it use
 * config.root to get to the file with version number.
 */
static int get_version(void)
{
	char path[PATH_MAX];
	FILE *file = NULL;
	int ret;
	int version;

	ret = snprintf(path, PATH_MAX, "%s/version", config.root);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	file = fopen(path, "r");
	if (!file) {
		/*
		 * First databases did not have version file at all.
		 * Hence it is like version 0.
		 */
		if (errno == ENOENT)
			return 0;

		perror(path);
		goto fail;
	}

	errno = 0;
	ret = fscanf(file, "%d", &version);

	if (ret == EOF && errno != 0) {
		perror(path);
		goto fail;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match database version number\n", path);
		goto fail;
	}

	fclose(file);

	return version;

fail:
	if (file)
		fclose(file);
	exit(EXIT_FAILURE);
}

void load_config(int check_version)
{
	char *tmp;

#define STRING(envname, value, fname) \
	if ((tmp = getenv(envname)))  \
		config.fname = tmp;
#define BOOL(envname, value, fname) \
	if ((tmp = getenv(envname)))  \
		config.fname = 1;
#include "default_config.h"
#undef BOOL
#undef STRING

	if (check_version) {
		int version = get_version();

		if (version > DATABASE_VERSION) {
			fprintf(stderr, "%s: Database layout unsupported, upgrade your teerank installation\n",
				config.root);
			exit(EXIT_FAILURE);
		} else if (version < DATABASE_VERSION) {
			fprintf(stderr, "%s: Database outdated, upgrade it with teerank-upgrade\n",
				config.root);
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
