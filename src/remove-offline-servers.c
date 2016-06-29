#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include "config.h"
#include "server.h"

/*
 * Cache the result of time() because the whole program wont run for
 * more than one second, so time() wont change.
 */
static long days_offline(time_t last_seen)
{
	static time_t now = (time_t)-1;

	if (now == (time_t)-1)
		now = time(NULL);

	return (now - last_seen) / (3600 * 24);
}

static int number_of_days(char *str)
{
	long ret;

	assert(str != NULL);

	errno = 0;
	ret = strtol(str, NULL, 10);
	if (errno != 0) {
		perror(str);
		exit(EXIT_FAILURE);
	}

	return ret;
}

int main(int argc, char **argv)
{
	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;
	long days;
	int dry_run = 0;
	unsigned count_offline = 0;

	load_config();

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "usage: %s <days> [dry-run]\n", argv[0]);
		return EXIT_FAILURE;
	}

	days = number_of_days(argv[1]);

	if (argc == 3) {
		if (strcmp(argv[2], "dry-run") != 0) {
			fprintf(stderr, "The option al third argument must be equal to 'dry-run'\n");
			return EXIT_FAILURE;
		}
		dry_run = 1;
	}

	sprintf(path, "%s/servers", config.root);
	if (!(dir = opendir(path)))
		return perror(path), EXIT_FAILURE;

	while ((dp = readdir(dir))) {
		struct server_state state;

		if (strcmp(".", dp->d_name) == 0 || strcmp("..", dp->d_name) == 0)
			continue;

		/* Just ignore server on error */
		if (!read_server_state(&state, dp->d_name))
			continue;

		if (days_offline(state.last_seen) >= days) {
			if (dry_run) {
				printf("'%s' would have been removed\n", dp->d_name);
			} else {
				remove_server(dp->d_name);
				verbose("Offline server removed: %s\n", dp->d_name);
			}
			count_offline++;
		}
	}

	verbose("%u offline servers removed\n", count_offline);

	closedir(dir);

	return EXIT_FAILURE;
}
