#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "info.h"
#include "config.h"
#include "server.h"
#include "clan.h"

int read_info(struct info *info)
{
	static const struct info INFO_ZERO;
	struct stat st;
	int ret;
	sqlite3_stmt *res;

	const struct {
		const char *query;
		unsigned *ptr;
	} *cinfo, COUNT_INFO[] = { {
			"SELECT COUNT(1) FROM players",
			&info->nplayers
		}, {
			"SELECT COUNT(1) FROM (SELECT DISTINCT clan FROM players WHERE" IS_VALID_CLAN ")",
			&info->nclans
		}, {
			"SELECT COUNT(1) FROM servers WHERE" IS_VANILLA_CTF_SERVER,
			&info->nservers
		}, { NULL }
	};

	*info = INFO_ZERO;

	for (cinfo = COUNT_INFO; cinfo->query; cinfo++) {
		if (sqlite3_prepare_v2(db, cinfo->query, -1, &res, NULL) != SQLITE_OK)
			goto fail;
		if (sqlite3_step(res) != SQLITE_ROW)
			goto fail;

		*cinfo->ptr = sqlite3_column_int64(res, 0);
		sqlite3_finalize(res);
	}

	/*
	 * "last_update" actually come from the last modification date
	 * of the database file.
	 */
	if (stat(config.dbpath, &st) == -1) {
		perror(config.dbpath);
		return 0;
	}
	info->last_update = st.st_mtim.tv_sec;

	struct master_info *master = info->masters;
	char master_query[] =
		"SELECT node, service, lastseen,"
		" (SELECT COUNT(1)"
		"  FROM servers"
		"  WHERE master_node = node"
		"  AND master_service = service)"
		" AS nservers"
		" FROM masters";

	if (sqlite3_prepare_v2(db, master_query, sizeof(master_query), &res, NULL) != SQLITE_OK)
		goto fail;

	while ((ret = sqlite3_step(res)) == SQLITE_ROW) {
		if (master - info->masters == MAX_MASTERS)
			break;

		snprintf(master->node, sizeof(master->node), "%s", sqlite3_column_text(res, 0));
		snprintf(master->service, sizeof(master->service), "%s", sqlite3_column_text(res, 1));

		master->lastseen = sqlite3_column_int64(res, 2);
		master->nservers = sqlite3_column_int64(res, 3);

		master++;
	}

	if (ret != SQLITE_ROW && ret != SQLITE_DONE)
		goto fail;

	info->nmasters = master - info->masters;

	sqlite3_finalize(res);

	return 1;

fail:
	fprintf(stderr, "%s: read_info(): %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}
