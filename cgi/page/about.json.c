#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "json.h"
#include "master.h"
#include "player.h"
#include "clan.h"
#include "server.h"

static void json_master(struct master *master)
{
	putchar('{');
	printf("\"node\":\"%s\",", master->node);
	printf("\"service\":\"%s\",", master->node);
	printf("\"last_seen\":\"%s\",", json_date(master->lastseen));
	printf("\"nservers\":%u", master->nservers);
	putchar('}');
}

static int json_info(void)
{
	int ret;
	unsigned n = 0;
	sqlite3_stmt *res;
	struct master master;
	const char query[] =
		"SELECT" ALL_MASTER_COLUMNS "," NSERVERS_COLUMN
		" FROM masters";

	putchar('{');

	printf("\"nplayers\":%u,", count_players());
	printf("\"nclans\":%u,",   count_clans());
	printf("\"nservers\":%u,", count_vanilla_servers());
	printf("\"last_update\":\"%s\",", json_date(last_database_update()));

	printf("\"masters\":[");

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	while ((ret = sqlite3_step(res)) == SQLITE_ROW) {
		if (n++)
			putchar(',');

		master_from_result_row(&master, res, 1);
		json_master(&master);
	}

	if (ret != SQLITE_DONE)
		goto fail;

	sqlite3_finalize(res);

	printf("],\"nmasters\":%u", n);

	putchar('}');
	return SUCCESS;
fail:
	fprintf(
		stderr, "%s: json_info(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return FAILURE;
}

int main_json_about(int argc, char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	return json_info();
}
