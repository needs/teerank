#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>

#include "config.h"
#include "player.h"
#include "master.h"

struct config config = {
#define STRING(envname, value, fname) \
	.fname = value,
#define BOOL(envname, value, fname) \
	.fname = value,
#define UNSIGNED(envname, value, fname) \
	.fname = value,
#include "default_config.h"
};

sqlite3 *db = NULL;

/*
 * Query database version.  It does require database handle to be
 * opened, as it will query version from the "version" table.
 */
static int get_version(void)
{
	int ret, version;
	sqlite3_stmt *res;
	const char query[] = "SELECT version FROM version";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	ret = sqlite3_step(res);
	if (ret == SQLITE_DONE)
		goto not_found;
	else if (ret != SQLITE_ROW)
		goto fail;

	version = sqlite3_column_int(res, 0);

	sqlite3_finalize(res);
	return version;

not_found:
	fprintf(stderr, "%s: Database doesn't contain a version number\n", config.dbpath);
	sqlite3_finalize(res);
	exit(EXIT_FAILURE);
fail:
	fprintf(stderr, "%s: get_version(): %s\n", config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	exit(EXIT_FAILURE);
}

static void init_version_table(void)
{
	sqlite3_stmt *res;
	const char query[] =
		"INSERT INTO version"
		" VALUES(?)";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_bind_int(res, 1, DATABASE_VERSION) != SQLITE_OK)
		goto fail;
	if (sqlite3_step(res) != SQLITE_DONE)
		goto fail;
	sqlite3_finalize(res);
	return;

fail:
	fprintf(
		stderr, "%s: init_version_table(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	exit(EXIT_FAILURE);
}

static void init_masters_table(void)
{
	sqlite3_stmt *res;
	const struct master *master;
	const char query[] =
		"INSERT INTO masters"
		" VALUES(?, ?, ?)";

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	for (master = DEFAULT_MASTERS; *master->node; master++) {
		if (sqlite3_bind_text(res, 1, master->node, -1, NULL) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_text(res, 2, master->service, -1, NULL) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_int64(res, 3, NEVER_SEEN) != SQLITE_OK)
			goto fail;

		if (sqlite3_step(res) != SQLITE_DONE)
			goto fail;

		if (sqlite3_reset(res) != SQLITE_OK)
			goto fail;
		if (sqlite3_clear_bindings(res) != SQLITE_OK)
			goto fail;
	}

	sqlite3_finalize(res);
	return;

fail:
	fprintf(
		stderr, "%s: init_masters_table(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	exit(EXIT_FAILURE);
}

static void create_database(void)
{
	const int FLAGS = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	sqlite3_stmt *res;

	const char **query = NULL, *queries[] = {
		"CREATE TABLE version("
		" version INTEGER,"
		" PRIMARY KEY(version))",

		"CREATE TABLE masters("
		" node TEXT,"
		" service TEXT,"
		" lastseen DATE,"
		" PRIMARY KEY(node, service))",

		"CREATE TABLE servers("
		" ip TEXT,"
		" port TEXT,"
		" name TEXT,"
		" gametype TEXT,"
		" map TEXT,"
		" lastseen DATE,"
		" expire DATE,"
		" master_node TEXT,"
		" master_service TEXT,"
		" max_clients INTEGER,"
		" PRIMARY KEY(ip, port),"
		" FOREIGN KEY(master_node, master_service)"
		"  REFERENCES masters(node, service))",

		"CREATE TABLE players("
		" name TEXT,"
		" clan TEXT,"
		" elo INTEGER,"
		" lastseen DATE,"
		" server_ip TEXT,"
		" server_port TEXT,"
		" PRIMARY KEY(name),"
		" FOREIGN KEY(server_ip, server_port)"
		"  REFERENCES servers(ip, port))",

		"CREATE TABLE server_clients("
		" ip TEXT,"
		" port TEXT,"
		" name TEXT,"
		" clan TEXT,"
		" score INTEGER,"
		" ingame BOOLEAN,"
		" PRIMARY KEY(ip, port, name),"
		" FOREIGN KEY(ip, port)"
		"  REFERENCES servers(ip, port)"
		" FOREIGN KEY(name)"
		"  REFERENCES players(name))",

		"CREATE TABLE player_historic("
		" name TEXT,"
		" timestamp DATE,"
		" elo INTEGER,"
		" rank INTEGER,"
		" PRIMARY KEY(name, timestamp),"
		" FOREIGN KEY(name)"
		"  REFERENCES players(name))",

		"CREATE INDEX players_by_rank"
		" ON players (elo DESC, lastseen DESC, name DESC)",

		"CREATE INDEX players_by_lastseen"
		" ON players (lastseen DESC, elo DESC, name DESC)",

		"CREATE INDEX clan_index"
		" ON players (clan)",

		NULL
	};

	if (sqlite3_open_v2(config.dbpath, &db, FLAGS, NULL) != SQLITE_OK)
		goto fail_open;

	/*
	 * Batch create queries in a single immediate transaction so
	 * that if someone else try to create the database at the same
	 * time, one will quietly fail, assuming the other successfully
	 * created the database.
	 */
	if (sqlite3_exec(db, "BEGIN IMMEDIATE", 0, 0, 0) != SQLITE_OK)
		return;

	for (query = queries; *query; query++) {
		if (sqlite3_prepare_v2(db, *query, -1, &res, NULL) != SQLITE_OK)
			goto fail;
		if (sqlite3_step(res) != SQLITE_DONE)
			goto fail;
		sqlite3_finalize(res);
	}

	init_version_table();
	init_masters_table();

	if (sqlite3_exec(db, "COMMIT", 0, 0, 0) != SQLITE_OK)
		goto fail;

	return;

fail_open:
	fprintf(
		stderr, "%s: create_database(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	exit(EXIT_FAILURE);

fail:
	fprintf(
		stderr, "%s: create_database(): query %zi: %s\n",
		config.dbpath, query - queries + 1, sqlite3_errmsg(db));
	sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	exit(EXIT_FAILURE);
}

void load_config(int check_version)
{
	char *tmp;

#define STRING(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = tmp;
#define BOOL(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = 1;
#define UNSIGNED(envname, value, fname) \
	if ((tmp = getenv(envname))) \
		config.fname = strtol(tmp, NULL, 10);

#include "default_config.h"

	/*
	 * Update delay too short makes the ranking innacurate and
	 * volatile, and when it's too long it adds too much
	 * uncertainties.  Between 1 and 20 seems to be reasonable.
	 */
	if (config.update_delay < 1) {
		fprintf(stderr, "%u: Update delay too short, ranking will be volatile\n",
		        config.update_delay);
		exit(EXIT_FAILURE);
	} else if (config.update_delay > 20) {
		fprintf(stderr, "%u: Update delay too long, data will be uncorrelated\n",
		        config.update_delay);
		exit(EXIT_FAILURE);
	}

	/* We work with UTC time only */
	setenv("TZ", "", 1);
	tzset();

	/* Open database now so we can check it's version */
	if (sqlite3_open_v2(config.dbpath, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK)
		create_database();

	/*
	 * Wait when two or more processes want to write the database.
	 * That is useful to avoid "database is locked", to some
	 * extents.  It could still happen if somehow the process has to
	 * wait more then the given timeout.
	 */
	sqlite3_busy_timeout(db, 5000);

	if (check_version) {
		int version = get_version();

		if (version > DATABASE_VERSION) {
			fprintf(stderr, "%s: Database too modern, upgrade your teerank installation\n",
				config.dbpath);
			exit(EXIT_FAILURE);
		} else if (version < DATABASE_VERSION) {
			fprintf(stderr, "%s: Database outdated, upgrade it with teerank-upgrade\n",
				config.dbpath);
			exit(EXIT_FAILURE);
		}
	}
}

void verbose(const char *fmt, ...)
{
	if (config.verbose) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}

time_t last_database_update(void)
{
	struct stat st;

	if (stat(config.dbpath, &st) == -1)
		return 0;

	return st.st_mtim.tv_sec;
}
