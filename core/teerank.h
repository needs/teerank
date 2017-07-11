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

/* Flags for init_teerank() */
enum {
	READ_ONLY  = (1 << 0),
	UPGRADABLE = (1 << 1)
};

/*
 * Load configuration from the environment, open the database and
 * perform some check as well.  Failure are fatal at this step so it
 * exit(EXIT_FAILURE) right away.
 */
void init_teerank(int flags);

/* Print the message only when TEERANK_VERBOSE is set */
void verbose(const char *fmt, ...);

/*
 * The following constant are from teeworlds source code, and they are
 * used to put an upper bound on our strings.
 */

#define NAME_STRSIZE 16
#define CLAN_STRSIZE 16

#define SERVERNAME_STRSIZE 256
#define GAMETYPE_STRSIZE 32
#define MAP_STRSIZE 64

#define IP_STRSIZE sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")
#define PORT_STRSIZE sizeof("00000")

#endif /* TEERANK_H */
