#ifndef CGI_H
#define CGI_H

#include <stdlib.h>
#include "database.h"
#include "io.h"

/* Used by pages */
#define EXIT_NOT_FOUND 2

void error(int code, char *fmt, ...);

extern struct cgi_config {
	const char *name;
	const char *port;
	char domain[1024];
} cgi_config;

/* PCS stands for Player Clan Server */
enum pcs {
	PCS_PLAYER,
	PCS_CLAN,
	PCS_SERVER
};

#define MAX_DIRS 8
#define MAX_ARGS 8

struct arg {
	char *name;
	char *val;
};

struct url {
	unsigned ndirs;
	char *dirs[MAX_DIRS];

	char *ext;

	unsigned nargs;
	struct arg args[MAX_ARGS];
};

unsigned parse_pnum(char *str);
unsigned char hextodec(char c);
void url_decode(char *str);

#define STRING_FIELD(name, size)                                        \
	char *name, name##_[size];

#define column_string(res, i, name)                                     \
	column_string_(res, i, name##_, sizeof(name##_))
char *column_string_(sqlite3_stmt *res, int i, char *buf, size_t size);

#endif /* CGI_H */
