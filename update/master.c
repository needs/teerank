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

static void read_master_(sqlite3_stmt *res, void *_m, bool extended)
{
	struct master *m = _m;

	column_text_copy(res, 0, m->node, sizeof(m->node));
	column_text_copy(res, 1, m->service, sizeof(m->service));

	m->lastseen = column_time_t(res, 2);
	m->expire = column_time_t(res, 3);

	if (extended)
		m->nservers = column_unsigned(res, 4);
}

void read_master(sqlite3_stmt *res, void *m)
{
	read_master_(res, m, 0);
}

void read_extended_master(sqlite3_stmt *res, void *m)
{
	read_master_(res, m, 1);
}

int write_master(struct master *m)
{
	const char *query =
		"INSERT OR REPLACE INTO masters"
		" VALUES (?, ?, ?, ?)";

	return exec(query, "sstt", m->node, m->service, m->lastseen, m->expire);
}
