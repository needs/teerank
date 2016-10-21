#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "page.h"

int main_json_player(int argc, char **argv)
{
	FILE *file = NULL;
	char path[PATH_MAX];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		goto fail;
	}

	if (!is_valid_hexname(argv[1]))
		return EXIT_NOT_FOUND;

	if (!dbpath(path, PATH_MAX, "players/%s", argv[1]))
		goto fail;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return EXIT_NOT_FOUND;
		perror(path);
		goto fail;
	}

	if (!dump_n_fields(file, 5))
		goto fail;
	putchar('}');

	fclose(file);
	return EXIT_SUCCESS;

fail:
	if (file)
		fclose(file);
	return EXIT_FAILURE;
}
