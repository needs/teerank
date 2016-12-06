#include <sqlite3.h>
#include <stdio.h>

#include "master.h"

const struct master DEFAULT_MASTERS[5] = {
	{ "master1.teeworlds.com", "8300" },
	{ "master2.teeworlds.com", "8300" },
	{ "master3.teeworlds.com", "8300" },
	{ "master4.teeworlds.com", "8300" },
	{ "", "" }
};

static void _read_master(sqlite3_stmt *res, void *_m, int extended)
{
	struct master *m = _m;

	snprintf(m->node, sizeof(m->node), "%s", sqlite3_column_text(res, 0));
	snprintf(m->service, sizeof(m->service), "%s", sqlite3_column_text(res, 1));

	m->lastseen = sqlite3_column_int64(res, 2);
	m->expire = sqlite3_column_int64(res, 3);

	if (extended)
		m->nservers = sqlite3_column_int64(res, 3);
}

void read_master(sqlite3_stmt *res, void *m)
{
	_read_master(res, m, 0);
}

void read_extended_master(sqlite3_stmt *res, void *m)
{
	_read_master(res, m, 1);
}

int write_master(struct master *m)
{
	const char query[] =
		"INSERT OR REPLACE INTO masters"
		" VALUES (?, ?, ?, ?)";

	return exec(query, "sstt", m->node, m->service, m->lastseen, m->expire);
}
