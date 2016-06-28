#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "4-to-5.h"
#include "config.h"

static void delete_pages(void)
{
	char dirpath[PATH_MAX], path[PATH_MAX];
	struct dirent *dp;
	DIR *dir = NULL;
	int ret;

	ret = snprintf(dirpath, PATH_MAX, "%s/pages", config.root);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	dir = opendir(dirpath);
	if (dir == NULL) {
		perror(dirpath);
		goto fail;
	}

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		ret = snprintf(path, PATH_MAX, "%s/pages/%s",
		               config.root, dp->d_name);
		if (ret >= PATH_MAX) {
			fprintf(stderr, "%s: Too long\n", config.root);
			goto fail;
		}

		ret = unlink(path);
		if (ret == -1) {
			perror(path);
			goto fail;
		}
	}

	closedir(dir);
	dir = NULL;

	ret = rmdir(dirpath);
	if (ret == -1) {
		perror(dirpath);
		goto fail;
	}

	return;

fail:
	if (dir)
		closedir(dir);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	load_config();

	upgrade_players();
	upgrade_ranks();
	delete_pages();

	return EXIT_SUCCESS;
}
