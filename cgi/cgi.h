#ifndef CGI_H
#define CGI_H

#include <stdlib.h>

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

int parse_pnum(char *str, unsigned *pnum);
unsigned char hextodec(char c);
void url_decode(char *str);

#endif /* CGI_H */
