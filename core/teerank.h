#ifndef TEERANK_H
#define TEERANK_H

/*
 * Used by many functions that search for a particular entity in the
 * database.  They use SUCCESS the entity has been successfully loaded ,
 * NOT_FOUND when the entity simply does not exist, and FAILURE
 * otherwise.
 *
 * They can be used as an exit status as well.
 */
enum {
	SUCCESS,
	FAILURE,
	NOT_FOUND
};

struct config {
#define STRING(envname, value, fname) \
	char *fname;
#define BOOL(envname, value, fname) \
	int fname;
#include "config.def"
};

extern struct config config;

/*
 * Load configuration from the environment, open the database and
 * perform some check as well.  Failure are fatal at this step so it
 * exit(EXIT_FAILURE) right away.
 */
void init_teerank(int readonly);

/* Print the message only when TEERANK_VERBOSE is set */
void verbose(const char *fmt, ...);

#endif /* TEERANK_H */
