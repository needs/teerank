#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>

#include "database.h"
#include "teerank.h"
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

/*
 * In WAL mode, database really have two file with relevant data that
 * can be read from.  The main file and the WAL file.  We need to check
 * both file to get the last modification date.
 */
time_t last_database_update(void)
{
	static char walfile[PATH_MAX];
	struct stat st;
	time_t db = 0, wal = 0;

	/*
	 * 'config.dbpath' isn't supposed to change, so set 'walfile'
	 * once for all.
	 */
	if (!walfile[0])
		snprintf(walfile, sizeof(walfile), "%s-wal", config.dbpath);

	if (stat(config.dbpath, &st) != -1)
		db = st.st_mtim.tv_sec;
	if (stat(walfile, &st) != -1)
		wal = st.st_mtim.tv_sec;

	return wal > db ? wal : db;
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

void create_all_indices(void)
{
	exec("CREATE INDEX ranks_by_gametype ON ranks (gametype, map, rank)");
	exec("CREATE INDEX players_by_clan ON players (clan)");
}

#define MAX_QUERY_STRSIZE (sizeof("DROP INDEX") + 32)

static void read_index_name(sqlite3_stmt *res, void *_query)
{
	char *query = _query;
	const char *name = (const char*)sqlite3_column_text(res, 0);
	snprintf(query, MAX_QUERY_STRSIZE, "DROP INDEX %s", name);
}

#define MAX_INDICES 16

/*
 * We want to drop old indices even unknown ones from an old database.
 * Thankfully Sqlite provide a way to query all indices names.  However,
 * we can't delete them while iterating on them.  So we need to store
 * them and then proceed to remove them.
 */
void drop_all_indices(void)
{
	sqlite3_stmt *res;
	unsigned nrow;

	char queries[MAX_INDICES][MAX_QUERY_STRSIZE];

	/*
	 * Avoid indices starting by "sqlite" because they are
	 * autoindex created for each table.
	 */
	const char *select =
		"SELECT name"
		" FROM sqlite_master"
		" WHERE type == 'index'"
		"  AND name NOT LIKE 'sqlite%'";

	foreach_row(select, read_index_name, queries[nrow]) {
		if (nrow + 1 == MAX_INDICES) {
			nrow++;
			break_foreach;
		}
	}

	while (nrow --> 0)
		exec(queries[nrow]);
}

static int create_database(void)
{
	const int FLAGS = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

	const char **query = NULL, *queries[] = {
		"CREATE TABLE version" TABLE_DESC_VERSION,
		"CREATE TABLE masters" TABLE_DESC_MASTERS,
		"CREATE TABLE servers" TABLE_DESC_SERVERS,
		"CREATE TABLE players" TABLE_DESC_PLAYERS,
		"CREATE TABLE server_clients" TABLE_DESC_SERVER_CLIENTS,
		"CREATE TABLE ranks" TABLE_DESC_RANKS,
		"CREATE TABLE ranks_historic" TABLE_DESC_RANK_HISTORIC,
		"CREATE TABLE pending" TABLE_DESC_PENDING,

		/* Note: Indices are created in create_all_indices() */
		NULL
	};

	if (sqlite3_open_v2(config.dbpath, &db, FLAGS, NULL) != SQLITE_OK) {
		errmsg("create_database", NULL);
		return 0;
	}

	/*
	 * By default, while recomputing_ranks() is running, the CGI
	 * cannot access the database.  This is a problem since it can
	 * take more than 10 seconds!
	 *
	 * That line enable Write-Ahead Logging, wich basically allow
	 * readers and writers to use the database at the same time.
	 * For us this is handy: CGI can access the database no matter
	 * what changes are made to the database.
	 *
	 * One drawback is that the CGI requires write access permission
	 * to the database and all extra file created by sqlite.  But as
	 * long as recompute_ranks() is slow, WAL is almost mandatory.
	 */
	exec("PRAGMA journal_mode=WAL");

	/*
	 * When in WAL mode, SQlite documentation explicitely advise
	 * using NORMAL instead of the default FULL.
	 */
	exec("PRAGMA synchronous=NORMAL");

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

	create_all_indices();

	if (!exec("COMMIT"))
		goto fail;

	return 1;

fail:
	exec("ROLLBACK");
	return 0;
}

static void close_database(void)
{
	/* Using NULL as a query free internal buffers exec() may keep */
	exec(NULL);

	if (sqlite3_close(db) != SQLITE_OK)
		errmsg("close_database", NULL);
}

int init_database(int readonly)
{
	int flags = SQLITE_OPEN_READWRITE;

	if (readonly)
		flags = SQLITE_OPEN_READONLY;

	if (sqlite3_open_v2(config.dbpath, &db, flags, NULL) != SQLITE_OK) {
		if (readonly) {
			errmsg("init_database", NULL);
			return 0;
		}

		if (!create_database())
			return 0;
	}

	/*
	 * We want to properly close the database connexion so that
	 * extra files created by SQlite are removed.  Also this may
	 * trigger a WAL checkpoint.
	 */
	atexit(close_database);

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

static void destroy_query(const char **prevquery, sqlite3_stmt **res)
{
	sqlite3_finalize(*res);
	*prevquery = NULL;
	*res = NULL;
}

int _exec(const char *query, const char *bindfmt, ...)
{
	static const char *prevquery;
	static sqlite3_stmt *res;

	va_list ap;
	int ret;

	/*
	 * NULL as a query allow to clean any prepared statement so that
	 * sqlite3_close() will not return SQLITE3_BUSY.
	 */
	if (!query) {
		destroy_query(&prevquery, &res);
		return 1;
	}

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
		sqlite3_finalize(*res);
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
