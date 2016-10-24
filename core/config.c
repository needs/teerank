#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "config.h"

struct config config = {
#define STRING(envname, value, fname) \
	.fname = value,
#define BOOL(envname, value, fname) \
	.fname = value,
#define UNSIGNED(envname, value, fname) \
	.fname = value,
#include "default_config.h"
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

	if (!dbpath(path, PATH_MAX, "version"))
		goto fail;

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

	/* Doing so makes mktime() usable for UTC time */
	setenv("TZ", "", 1);
	tzset();
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

static char *prefix_root(char *buf, size_t *size)
{
	int ret;

	assert(buf != NULL);
	assert(size != NULL);

	ret = snprintf(buf, *size, "%s/", config.root);
	if (ret < 0) {
		perror(config.root);
		return NULL;
	} else if (ret >= *size) {
		fprintf(stderr, "%s: Too long\n", config.root);
		return NULL;
	}

	*size -= ret;
	return buf + ret;
}

char *dbpath(char *buf, size_t size, const char *fmt, ...)
{
	char *tmp;
	va_list ap;
	int ret;

	/* Prefix config.root */
	if (!(tmp = prefix_root(buf, &size)))
		return NULL;

	va_start(ap, fmt);
	ret = vsnprintf(tmp, size, fmt, ap);
	if (ret < 0) {
		perror(config.root);
		goto fail;
	} else if (ret >= size) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}
	va_end(ap);
	return buf;

fail:
	va_end(ap);
	return NULL;
}
