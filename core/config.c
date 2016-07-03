#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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

void load_config(void)
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
