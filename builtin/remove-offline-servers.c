#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "server.h"

/*
 * Cache the result of time() because the whole program wont run for
 * more than one second, so time() wont change.
 */
static long days_offline(time_t lastseen)
{
	static time_t now = (time_t)-1;

	if (lastseen == NEVER_SEEN)
		return 0;

	if (now == (time_t)-1)
		now = time(NULL);

	return (now - lastseen) / (3600 * 24);
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

static int remove_offline_servers(long days, int dryrun)
{
	struct server server;
	sqlite3_stmt *res;
	unsigned nrow, offline = 0;
	const char query[] =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers";

	exec("BEGIN");

	foreach_server(query, &server) {
		if (days_offline(server.lastseen) >= days) {
			char *addr = build_addr(server.ip, server.port);

			if (dryrun) {
				printf("'%s' would have been removed\n", addr);
			} else {
				remove_server(server.ip, server.port);
				verbose("Offline server removed: %s\n", addr);
			}
			offline++;
		}
	}

	exec("COMMIT");

	if (!res)
		return FAILURE;

	verbose("%u offline servers removed\n", offline);
	return SUCCESS;
}

int main(int argc, char **argv)
{
	long days;

	load_config(1);

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
		return remove_offline_servers(days, 1);
	} else {
		return remove_offline_servers(days, 0);
	}

}
