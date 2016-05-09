#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "config.h"

/*
 * Default values are for local use (ie when working on source code) rather
 * than global use (using the CGI with /var/...).  This is done to make it
 * easy to hack on it, since everything will work after git clone && make
 * without any extra steps.
 */
struct config config = {
	.root = ".teerank",
	.verbose = 0,
	.debug = 0
};

void load_config(void)
{
	char *tmp;

	if ((tmp = getenv("TEERANK_ROOT")))
		config.root = tmp;
	if ((tmp = getenv("TEERANK_VERBOSE")))
		config.verbose = 1;
	if ((tmp = getenv("TEERANK_DEBUG")))
		config.debug = 1;
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
