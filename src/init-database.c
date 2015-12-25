#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>

#include "config.h"

static int error = 0;

static void create_directory(char *fmt, ...)
{
	va_list ap;
	char path[PATH_MAX];
	int ret;

	va_start(ap, fmt);
	vsprintf(path, fmt, ap);
	va_end(ap);

	if ((ret = mkdir(path, 0777)) == -1 && errno != EEXIST)
		perror(path), error = 1;
}

int main(int argc, char **argv)
{
	load_config();

	create_directory("%s", config.root);
	create_directory("%s/servers", config.root);
	create_directory("%s/players", config.root);
	create_directory("%s/clans", config.root);
	create_directory("%s/pages", config.root);

	return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
