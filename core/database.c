#include <stdlib.h>
#include <stdio.h>

#include "database.h"
#include "config.h"
#include "master.h"
#include "player.h"

sqlite3 *db = NULL;

int database_version(void)
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

static int init_version_table(void)
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
	return 1;

fail:
	fprintf(
		stderr, "%s: init_version_table(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	return 0;
}

static int init_masters_table(void)
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
	return 1;

fail:
	fprintf(
		stderr, "%s: init_masters_table(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	return 0;
}

int create_database(void)
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
		return 1;

	for (query = queries; *query; query++) {
		if (sqlite3_prepare_v2(db, *query, -1, &res, NULL) != SQLITE_OK)
			goto fail;
		if (sqlite3_step(res) != SQLITE_DONE)
			goto fail;
		sqlite3_finalize(res);
	}

	if (!init_version_table())
		goto fail_init;
	if (!init_masters_table())
		goto fail_init;

	if (sqlite3_exec(db, "COMMIT", 0, 0, 0) != SQLITE_OK)
		goto fail;

	return 1;

fail_open:
	fprintf(
		stderr, "%s: create_database(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	return 0;

fail:
	fprintf(
		stderr, "%s: create_database(): query %zi: %s\n",
		config.dbpath, query - queries + 1, sqlite3_errmsg(db));
	sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	return 0;

fail_init:
	fprintf(
		stderr, "%s: create_database(): init: %s\n",
		config.dbpath, sqlite3_errmsg(db));
	sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	return 0;
}

static void errmsg(const char *func, const char *query)
{
	const char *dbpath = config.dbpath;
	const char *msg = sqlite3_errmsg(db);

#ifdef NDEBUG
	fprintf(stderr, "%s: %s: %s\n", dbpath, func, msg);
#else
	if (query)
		fprintf(stderr, "%s: %s: %s: %s\n", dbpath, func, query, msg);
	else
		fprintf(stderr, "%s: %s: %s\n", dbpath, func, msg);
#endif
}

void foreach_init(sqlite3_stmt **res, const char *query, const char *bindfmt, ...)
{
	va_list ap;
	unsigned i;
	int ret = sqlite3_prepare_v2(db, query, -1, res, NULL);

	if (ret != SQLITE_OK) {
		errmsg("foreach_init", query);
		*res = NULL;
		return;
	}

	va_start(ap, bindfmt);

	for (i = 0; bindfmt[i]; i++) {
		switch (bindfmt[i]) {
		case 'i':
			ret = sqlite3_bind_int(*res, i+1, va_arg(ap, int));
			break;

		case 'u':
			ret = sqlite3_bind_int64(*res, i+1, va_arg(ap, int));
			break;

		case 's':
			ret = sqlite3_bind_text(*res, i+1, va_arg(ap, char*), -1, SQLITE_STATIC);
			break;
		}

		if (ret != SQLITE_OK) {
			va_end(ap);
			errmsg("foreach_init", query);
			sqlite3_finalize(*res);
			*res = NULL;
			return;
		}
	}

	va_end(ap);
}

int foreach_next(sqlite3_stmt **res, void *data, void (*read_row)(sqlite3_stmt*, void*))
{
	int ret = sqlite3_step(*res);

	if (!*res)
		return 0;

	if (ret == SQLITE_DONE) {
		sqlite3_finalize(*res);
		return 0;
	} else if (ret != SQLITE_ROW) {
		*res = NULL;
		errmsg("foreach_next", NULL);
		sqlite3_finalize(*res);
		return 0;
	} else {
		read_row(*res, data);
		return 1;
	}
}
