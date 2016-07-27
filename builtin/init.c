#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"

static void create_dir(const char *filename)
{
	char path[PATH_MAX];
	int ret;

	if (!dbpath(path, PATH_MAX, "%s", filename))
		exit(EXIT_FAILURE);

	ret = mkdir(path, 0777);
	if (ret == -1 && errno != EEXIST) {
		perror(path);
		exit(EXIT_FAILURE);
	}
}

static void set_database_version(unsigned version)
{
	char path[PATH_MAX];
	FILE *file;
	int ret;

	if (!dbpath(path, PATH_MAX, "version"))
		exit(EXIT_FAILURE);

	if (access(path, F_OK) == 0)
		return;

	file = fopen(path, "w");
	if (!file) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	ret = fprintf(file, "%u", version);
	if (ret < 0) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	fclose(file);
}

int main(int argc, char *argv[])
{
	/* Since database may not exist, checking version is useless */
	load_config(0);

	create_dir("");
	create_dir("servers");
	create_dir("players");
	create_dir("clans");

	set_database_version(DATABASE_VERSION);

	return EXIT_SUCCESS;
}
