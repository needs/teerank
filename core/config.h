#ifndef CONFIG_H
#define CONFIG_H

/* TODO: Move this to some kind of CGI header only */
#define EXIT_NOT_FOUND 2

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

void load_config(void);
void verbose(const char *fmt, ...);

#endif /* CONFIG_H */
