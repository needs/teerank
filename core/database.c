#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "database.h"
#include "config.h"
#include "master.h"
#include "player.h"

sqlite3 *db = NULL;

static void read_version(sqlite3_stmt *res, void *_version)
{
	int *version = _version;
	*version = sqlite3_column_int(res, 0);
}

int database_version(void)
{
	int version;
	sqlite3_stmt *res;
	unsigned nrow;

	const char *query = "SELECT version FROM version";

	foreach_row(query, read_version, &version);

	if (!res)
		exit(EXIT_FAILURE);

	if (!nrow) {
		fprintf(stderr, "%s: Database doesn't contain a version number\n", config.dbpath);
		exit(EXIT_FAILURE);
	}

	return version;
}

static int init_version_table(void)
{
	const char *query =
		"INSERT INTO version"
		" VALUES(?)";

	return exec(query, "i", DATABASE_VERSION);
}

static int bind(sqlite3_stmt *res, const char *bindfmt, va_list ap)
{
	unsigned i;
	int ret;

	for (i = 0; bindfmt[i]; i++) {
		switch (bindfmt[i]) {
		case 'i':
			ret = sqlite3_bind_int(res, i+1, va_arg(ap, int));
			break;

		case 'u':
			ret = sqlite3_bind_int64(res, i+1, va_arg(ap, unsigned));
			break;

		case 's':
			ret = sqlite3_bind_text(res, i+1, va_arg(ap, char*), -1, SQLITE_STATIC);
			break;

		case 't':
			ret = sqlite3_bind_int64(res, i+1, (unsigned)va_arg(ap, time_t));
			break;

		default:
			return 0;
		}

		if (ret != SQLITE_OK)
			return 0;
	}

	return 1;
}

static int init_masters_table(void)
{
	const struct master *master;
	int ret = 1;

	const char *query =
		"INSERT INTO masters"
		" VALUES(?, ?, ?, ?)";

	for (master = DEFAULT_MASTERS; *master->node; master++)
		ret &= exec(query, "sstt", master->node, master->service, NEVER_SEEN, EXPIRE_NOW);

	return ret;
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

static int create_database(void)
{
	const int FLAGS = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

	const char **query = NULL, *queries[] = {
		"CREATE TABLE version("
		" version INTEGER,"
		" PRIMARY KEY(version))",

		"CREATE TABLE masters("
		" node TEXT,"
		" service TEXT,"
		" lastseen DATE,"
		" expire DATE,"
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
		" rank UNSIGNED,"
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

		"CREATE TABLE pending("
		" name TEXT,"
		" elo INTEGER,"
		" PRIMARY KEY(name),"
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

	if (sqlite3_open_v2(config.dbpath, &db, FLAGS, NULL) != SQLITE_OK) {
		errmsg("create_database", NULL);
		return 0;
	}

	/*
	 * Batch create queries in a single exclusive transaction so
	 * that if someone else try to use the database at the same
	 * time, one will quietly wait for the other to create the
	 * database.
	 */
	if (sqlite3_exec(db, "BEGIN EXCLUSIVE", 0, 0, 0) != SQLITE_OK)
		return 1;

	for (query = queries; *query; query++) {
		if (!exec(*query))
			goto fail;
	}

	if (!init_version_table())
		goto fail;
	if (!init_masters_table())
		goto fail;

	if (!exec("COMMIT"))
		goto fail;

	return 1;

fail:
	exec("ROLLBACK");
	return 0;
}

int init_database(int readonly)
{
	int flags = SQLITE_OPEN_READWRITE;

	if (readonly)
		flags = SQLITE_OPEN_READONLY;

	if (sqlite3_open_v2(config.dbpath, &db, flags, NULL) != SQLITE_OK) {
		if (readonly)
			return 0;
		else
			return create_database();
	}

	return 1;
}

unsigned _count_rows(const char *query, const char *bindfmt, ...)
{
	va_list ap;
	unsigned ret, count;
	struct sqlite3_stmt *res;

	if (sqlite3_prepare_v2(db, query, -1, &res, NULL) != SQLITE_OK)
		goto fail;

	va_start(ap, bindfmt);
	ret = bind(res, bindfmt, ap);
	va_end(ap);

	if (!ret)
		goto fail;
	if (sqlite3_step(res) != SQLITE_ROW)
		goto fail;

	count = sqlite3_column_int64(res, 0);

	sqlite3_finalize(res);
	return count;

fail:
	errmsg("count_rows", query);
	sqlite3_finalize(res);
	return 0;
}

/*
 * If the given query is the same than the previous one, we want to
 * reset it and clear any bindings instead of re-preparing it: that's
 * faster.
 */
static int prepare_query(const char *query, const char **prevquery, sqlite3_stmt **res)
{
	assert(query);
	assert(!*prevquery || *res);

	/*
	 * Failing to reset state and bindings is not fatal: we can
	 * still fallback and try to re-prepare the query.
	 */
	if (query == *prevquery) {
		if (sqlite3_reset(*res) != SQLITE_OK)
			goto prepare;
		if (sqlite3_clear_bindings(*res) != SQLITE_OK)
			goto prepare;

		return 1;
	}

prepare:
	if (*prevquery)
		sqlite3_finalize(*res);

	if (sqlite3_prepare_v2(db, query, -1, res, NULL) != SQLITE_OK) {
		*prevquery = NULL;
		return 0;
	}

	*prevquery = query;
	return 1;
}

int _exec(const char *query, const char *bindfmt, ...)
{
	static const char *prevquery;
	static sqlite3_stmt *res;

	va_list ap;
	int ret;

	assert(query != NULL);

	if (!prepare_query(query, &prevquery, &res))
		goto fail;

	va_start(ap, bindfmt);
	ret = bind(res, bindfmt, ap);
	va_end(ap);

	if (!ret)
		goto fail;

	ret = sqlite3_step(res);
	if (ret != SQLITE_ROW && ret != SQLITE_DONE)
		goto fail;

	return 1;

fail:
	errmsg("exec", query);
	return 0;
}

sqlite3_stmt *foreach_init(const char *query, const char *bindfmt, ...)
{
	int ret;
	va_list ap;
	sqlite3_stmt *res;

	if (sqlite3_prepare_v2(db, query, -1, &res, NULL))
		goto fail;

	va_start(ap, bindfmt);
	ret = bind(res, bindfmt, ap);
	va_end(ap);

	if (!ret)
		goto fail;

	return res;
fail:
	errmsg("foreach_init", query);
	sqlite3_finalize(res);
	return NULL;
}

int foreach_next(sqlite3_stmt **res, void *data, void (*read_row)(sqlite3_stmt*, void*))
{
	if (!*res)
		return 0;

	int ret = sqlite3_step(*res);

	if (ret == SQLITE_DONE) {
		return 0;
	} else if (ret == SQLITE_ROW) {
		if (read_row)
			read_row(*res, data);
		return 1;
	} else {
		errmsg("foreach_next", NULL);
		sqlite3_finalize(*res);
		*res = NULL;
		return 0;
	}
}
