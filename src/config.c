#include <stdlib.h>

#include "config.h"

/*
 * Default values are for local use (ie when working on source code) rather
 * than global use (using the CGI with /var/...).  This is done to make it
 * easy to hack on it, since everything will work after git clone && make
 * without any extra steps.
 */
struct config config = {
	.root = "data",
	.cache_root = ".",
};

void load_config(void)
{
	char *tmp;

	if ((tmp = getenv("TEERANK_ROOT")))
		config.root = tmp;
	if ((tmp = getenv("TEERANK_CACHE_ROOT")))
		config.cache_root = tmp;
}
