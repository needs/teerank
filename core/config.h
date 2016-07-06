#ifndef CONFIG_H
#define CONFIG_H

struct config {
#define STRING(envname, value, fname) \
	char *fname;
#define BOOL(envname, value, fname) \
	int fname;
#include "default_config.h"
#undef BOOL
#undef STRING
};

extern struct config config;

/**
 * Load configuration from environement.  It shoudld be called at the
 * start of every program that use the config structure.
 *
 * Set check_version to 1 if your program isn't called too often,
 * because checking version is a little bit expensive.  You might not
 * want to do it for a program on the fast path.
 *
 * It does exit(EXIT_FAILURE) when version cannot be checked.  However
 * it cannot fail when check_version is not set.
 *
 * @param check_version 1 to make sure database is up-to-date
 */
void load_config(int check_version);

void verbose(const char *fmt, ...);

#endif /* CONFIG_H */
