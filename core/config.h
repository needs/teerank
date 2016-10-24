#ifndef CONFIG_H
#define CONFIG_H

struct config {
#define STRING(envname, value, fname) \
	char *fname;
#define BOOL(envname, value, fname) \
	int fname;
#define UNSIGNED(envname, value, fname) \
	unsigned fname;
#include "default_config.h"
};

/*
 * Used by many functions that search for a particular entity in the
 * database.  It is needed to have diffreent code whenever the entity
 * exist but cannot be loaded (FAILURE), and when the entity simply does
 * not exist.
 *
 * I can be used directly as an exit status.
 */
enum {
	SUCCESS,
	FAILURE,
	NOT_FOUND
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

/**
 * Copy the full path to access the given file or directory in the
 * database in the provided buffer.
 *
 * It is recommended to use PATH_MAX for buffer size.
 *
 * @param buf A valid buffer
 * @param size Size of the given buffer
 * @param fmt See printf() format string
 * @return Return buf on success, NULL on failure
 */
char *dbpath(char *buf, size_t size, const char *fmt, ...);

#endif /* CONFIG_H */
