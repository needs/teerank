#ifndef CGI_H
#define CGI_H

#include "route.h"
#include <stdlib.h>

/* Used by pages */
#define EXIT_NOT_FOUND 2

void error(int code, char *fmt, ...);
void redirect(char *url);

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

int parse_pnum(char *str, unsigned *pnum);

unsigned char hextodec(char c);

void url_decode(char *str);

#endif /* CGI_H */
