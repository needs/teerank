#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "page.h"

int main_json_clan(int argc, char **argv)
{
	char path[PATH_MAX];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!is_valid_hexname(argv[1]))
		return EXIT_NOT_FOUND;

	if (!dbpath(path, PATH_MAX, "clans/%s", argv[1]))
		return EXIT_FAILURE;

	return dump(path);
}
