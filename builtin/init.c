#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"

static char *get_path(const char *filename)
{
	static char path[PATH_MAX];
	int ret;

	ret = snprintf(path, PATH_MAX, "%s/%s", config.root, filename);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		exit(EXIT_FAILURE);
	}

	return path;
}

static void create_dir(const char *filename)
{
	char *path;
	int ret;

	path = get_path(filename);

	ret = mkdir(path, 0777);
	if (ret == -1 && errno != EEXIST) {
		perror(path);
		exit(EXIT_FAILURE);
	}
}

static void set_database_version(unsigned version)
{
	char *path;
	FILE *file;
	int ret;

	path = get_path("version");

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
	load_config();

	create_dir("");
	create_dir("servers");
	create_dir("players");
	create_dir("clans");

	set_database_version(5);

	return EXIT_SUCCESS;
}
