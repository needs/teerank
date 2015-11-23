#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#include "delta.h"

static char *join(char *dir1, char *dir2, char *filename)
{
	static char path[PATH_MAX];
	sprintf(path, "%s/%s/%s", dir1, dir2, filename);
	return path;
}

static char *dir;

enum status {
	ERROR = 0,
	NEW, EXIST
};

static enum status create_directory(struct player_delta *player)
{
	char *path;

	assert(player != NULL);

	path = join(dir, player->name, "");

	if (mkdir(path, S_IRWXU) == -1) {
		if (errno == EEXIST)
			return EXIST;
		perror(path);
		return ERROR;
	}

	return NEW;
}

static void write_file(char *path, char *str)
{
	FILE *file;

	assert(path != NULL);

	if (!(file = fopen(path, "w")))
		return perror(path);
	fputs(str, file);
	fclose(file);
}

int main(int argc, char **argv)
{
	struct delta delta;
	unsigned i;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	dir = argv[1];

	while (scan_delta(&delta)) {
		for (i = 0; i < delta.length; i++) {
			struct player_delta *player = &delta.players[i];
			enum status status = create_directory(player);

			if (status == ERROR)
				continue;
			else if (status == NEW) {
				write_file(join(dir, player->name, "name"), player->name);
				write_file(join(dir, player->name, "elo"), "1500");
			}
			write_file(join(dir, player->name, "clan"), player->clan);
		}

		/* Print delta to allow piping */
		print_delta(&delta);
	}

	return EXIT_SUCCESS;
}
