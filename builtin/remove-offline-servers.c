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
	int ret;
	sqlite3_stmt *res;
	unsigned offline = 0;
	const char query[] =
		"SELECT" ALL_SERVER_COLUMN
		" FROM servers";

	sqlite3_exec(db, "BEGIN", 0, 0, 0);

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	while ((ret = sqlite3_step(res)) == SQLITE_ROW) {
		struct server server;

		server_from_result_row(&server, res, 0);

		if (days_offline(server.lastseen) >= days) {
			if (dryrun) {
				printf("'%s' would have been removed\n", build_addr(server.ip, server.port));
			} else {
				remove_server(server.ip, server.port);
				verbose("Offline server removed: %s\n", build_addr(server.ip, server.port));
			}
			offline++;
		}
	}

	if (ret != SQLITE_DONE)
		goto fail;

	sqlite3_exec(db, "END", 0, 0, 0);

	verbose("%u offline servers removed\n", offline);

	sqlite3_finalize(res);
	return SUCCESS;

fail:
	fprintf(
		stderr, "%s: remove_offline_servers(): %s\n",
	        config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return FAILURE;
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
