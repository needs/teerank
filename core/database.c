#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>

#include "database.h"
#include "teerank.h"

sqlite3 *db = NULL;
int dberr = 0;

int database_version(void)
{
	sqlite3_stmt *res;
	int version = -1;

	foreach_row(res, "SELECT version FROM version")
		version = column_int(res, 0);

	if (dberr)
		exit(EXIT_FAILURE);

	if (version == -1) {
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

static void init_version_table(void)
{
	exec("INSERT INTO version VALUES(?)", "i", DATABASE_VERSION);
}

static int bind_text(sqlite3_stmt *res, unsigned i, char *text)
{
	if (text)
		return sqlite3_bind_text(res, i, text, -1, SQLITE_STATIC);
	else
		return sqlite3_bind_null(res, i);
}

static int bind(sqlite3_stmt *res, const char *bindfmt, va_list ap)
{
	unsigned i;
	int ret;

	if (!bindfmt)
		return 1;

	for (i = 0; bindfmt[i]; i++) {
		switch (bindfmt[i]) {
		case 'i':
			ret = sqlite3_bind_int(res, i+1, va_arg(ap, int));
			break;

		case 'u':
			ret = sqlite3_bind_int64(res, i+1, va_arg(ap, unsigned));
			break;

		case 's':
			ret = bind_text(res, i+1, va_arg(ap, char*));
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

static void init_masters_table(void)
{
	const struct master {
		char *node;
		char *service;
	} *master = (struct master[]){
		{ "master1.teeworlds.com", "8300" },
		{ "master2.teeworlds.com", "8300" },
		{ "master3.teeworlds.com", "8300" },
		{ "master4.teeworlds.com", "8300" },
		{ NULL }
	};

	for (; master->node; master++) {
		exec(
			"INSERT INTO masters VALUES(?, ?, 0, 0)",
			"ss", master->node, master->service
		);
	}
}

static void errmsg(const char *func, const char *query)
{
	const char *dbpath = config.dbpath;
	const char *msg = sqlite3_errmsg(db);

	/*
	 * An error often cause other queries to fail where they would
	 * normally have succeded.  So we don't print the error message
	 * when we already have printed one.
	 */
	if (dberr++)
		return;

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
	exec("CREATE INDEX IF NOT EXISTS ranks_by_gametype ON ranks (gametype, map, rank)");
	exec("CREATE INDEX IF NOT EXISTS players_by_clan ON players (clan)");
}

/*
 * We can query the list of all indices by looking in the
 * "sqlite_master" table.  However, we can't delete indices while using
 * "sqlite_master".  So we store indices names and then we drop them.
 */
void drop_all_indices(void)
{
	char qs[16][64];
	sqlite3_stmt *res;
	unsigned i = 0;

	/*
	 * Avoid indices starting by "sqlite" because they are
	 * autoindex created for each table.
	 */
	const char *select =
		"SELECT name"
		" FROM sqlite_master"
		" WHERE type == 'index'"
		"  AND name NOT LIKE 'sqlite%'";

	foreach_row(res, select) {
		if (i < 16) {
			char *name = column_text(res, 0);
			snprintf(qs[i], sizeof(qs[i]), "DROP INDEX %s", name);
			i++;
		}
	}

	while (i --> 0)
		exec(qs[i]);
}

/*
 * If every tables are already created, this basically does nothing.  If
 * for some reason a table is missing, it will be recreated.
 */
static bool create_database(void)
{
	const char **query = NULL, *queries[] = {
		"CREATE TABLE IF NOT EXISTS version" TABLE_DESC_VERSION,
		"CREATE TABLE IF NOT EXISTS masters" TABLE_DESC_MASTERS,
		"CREATE TABLE IF NOT EXISTS servers" TABLE_DESC_SERVERS,
		"CREATE TABLE IF NOT EXISTS players" TABLE_DESC_PLAYERS,
		"CREATE TABLE IF NOT EXISTS server_clients" TABLE_DESC_SERVER_CLIENTS,
		"CREATE TABLE IF NOT EXISTS server_masters" TABLE_DESC_SERVER_MASTERS,
		"CREATE TABLE IF NOT EXISTS ranks" TABLE_DESC_RANKS,
		"CREATE TABLE IF NOT EXISTS ranks_historic" TABLE_DESC_RANK_HISTORIC,
		"CREATE TABLE IF NOT EXISTS pending" TABLE_DESC_PENDING,
		NULL

		/* Note: Indices are created in create_all_indices() */
	};

	exec("BEGIN");

	for (query = queries; *query; query++)
		exec(*query);

	init_version_table();
	init_masters_table();
	create_all_indices();

	if (dberr)
		return exec("ROLLBACK"), false;
	else
		return exec("COMMIT"), true;
}

static void close_database(void)
{
	/* Using NULL as a query free internal buffers exec() may keep */
	exec(NULL);

	if (sqlite3_close(db) != SQLITE_OK)
		errmsg("close_database", NULL);
}

bool init_database(bool readonly)
{
	int flags;

	if (readonly)
		flags = SQLITE_OPEN_READONLY;
	else
		flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

	if (sqlite3_open_v2(config.dbpath, &db, flags, NULL) != SQLITE_OK) {
		errmsg("init_database", NULL);
		return false;
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

	if (!readonly && !create_database())
		return false;

	/*
	 * We want to properly close the database connexion so that
	 * extra files created by SQlite are removed.  Also this may
	 * trigger a WAL checkpoint.
	 */
	atexit(close_database);

	return true;
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

bool exec_(const char *query, const char *bindfmt, ...)
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
	sqlite3_stmt *res;
	va_list ap;

	va_start(ap, bindfmt);
	res = foreach_init_(query, bindfmt, ap);
	va_end(ap);
	return res;
}

sqlite3_stmt *foreach_init_(const char *query, const char *bindfmt, va_list ap)
{
	sqlite3_stmt *res;

	if (sqlite3_prepare_v2(db, query, -1, &res, NULL))
		goto fail;
	if (!bind(res, bindfmt, ap))
		goto fail;

	return res;
fail:
	errmsg("foreach_init", query);
	sqlite3_finalize(res);
	return NULL;
}

sqlite3_stmt *foreach_next(sqlite3_stmt *res)
{
	int ret = sqlite3_step(res);

	if (ret == SQLITE_ROW) {
		return res;
	} else if (ret == SQLITE_DONE) {
		sqlite3_finalize(res);
		return NULL;
	} else {
		errmsg("foreach_next", NULL);
		sqlite3_finalize(res);
		return NULL;
	}
}

char *column_text_copy(sqlite3_stmt *res, int i, char *buf, size_t size)
{
	char *text = column_text(res, i);

	if (!text)
		return NULL;
	if (!buf)
		return text;

	snprintf(buf, size, "%s", text);
	return buf;
}

bool is_column_null(sqlite3_stmt *res, int i)
{
	return sqlite3_column_type(res, i) == SQLITE_NULL;
}

unsigned count_rows_(const char *query, const char *bindfmt, ...)
{
	va_list ap;
	unsigned ret;

	va_start(ap, bindfmt);
	ret = count_rows__(query, bindfmt, ap);
	va_end(ap);
	return ret;
}

unsigned count_rows__(const char *query, const char *bindfmt, va_list ap)
{
	unsigned count;
	struct sqlite3_stmt *res;

	if (sqlite3_prepare_v2(db, query, -1, &res, NULL) != SQLITE_OK)
		goto fail;
	if (!bind(res, bindfmt, ap))
		goto fail;
	if (sqlite3_step(res) != SQLITE_ROW)
		goto fail;

	count = column_unsigned(res, 0);

	sqlite3_finalize(res);
	return count;

fail:
	errmsg("count_rows", query);
	sqlite3_finalize(res);
	return 0;
}
