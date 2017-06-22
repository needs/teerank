#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"

enum {
	STATUS_OK,
	STATUS_DOWN,
	STATUS_STOPPED,
	STATUS_ENABLED,
	STATUS_DISABLED,
	STATUS_UNKNOWN
};

static const char *status_class(int status)
{
	switch (status) {
	case STATUS_OK:
		return "ok";
	case STATUS_DOWN:
		return "down";
	case STATUS_STOPPED:
		return "stopped";
	case STATUS_ENABLED:
		return "enabled";
	case STATUS_DISABLED:
		return "disabled";
	case STATUS_UNKNOWN:
	default:
		return "unknown";
	}
}

static const char *status_text(int status)
{
	switch (status) {
	case STATUS_OK:
		return "OK";
	case STATUS_DOWN:
		return "Down";
	case STATUS_STOPPED:
		return "Stopped";
	case STATUS_ENABLED:
		return "Enabled";
	case STATUS_DISABLED:
		return "Disabled";
	case STATUS_UNKNOWN:
	default:
		return "Unknown";
	}
}

static void print_status(const char *title, const char *comment, int status)
{
	assert(title != NULL);

	html("<section class=\"status status_%S\">", status_class(status));
	html("<span>%s</span>", title);
	if (comment)
		html("<span class=\"comment\">%s</span>", comment);
	html("<span>%s</span>", status_text(status));
	html("</section>");
}

struct master {
	char *node;
	time_t lastseen;
	unsigned nservers;
};

static void read_master(sqlite3_stmt *res, struct master *master)
{
	master->node = column_text(res, 0);
	master->lastseen = column_time_t(res, 1);
	master->nservers = column_unsigned(res, 2);
}

static int show_masters_status(int teerank_stopped)
{
	struct sqlite3_stmt *res;
	char buf[16], comment[64];

	const char *query =
		"SELECT node, lastseen,"
		" (SELECT COUNT(1)"
		"  FROM servers"
		"  WHERE master_node = node"
		"  AND master_service = service)"
		" AS nservers "
		" FROM masters"
		" ORDER BY node";

	html("<h2>Teeworlds</h2>");

	foreach_row(res, query) {
		struct master m;
		read_master(res, &m);

		/*
		 * If teerank is stopped, then we can't really guess
		 * masters status.
		 */
		if (teerank_stopped)
			print_status(m.node, NULL, STATUS_UNKNOWN);

		else if (!m.lastseen)
			print_status(m.node, NULL, STATUS_DOWN);

		else if (elapsed_time(m.lastseen, NULL, buf, sizeof(buf))) {
			snprintf(comment, sizeof(comment), "Since %s", buf);
			print_status(m.node, comment, STATUS_DOWN);

		} else {
			snprintf(comment, sizeof(comment), "%u servers", m.nservers);
			print_status(m.node, comment, STATUS_OK);
		}
	}

	if (res)
		return 1;

	return 0;
}

void generate_html_status(struct url *url)
{
	const char *title;
	char buf[16], comment[64];
	short teerank_stopped = 0;

	html_header("Status", "Status", "", NULL);

	html("<h2>Teerank</h2>");

	title = "Teerank";
	if (elapsed_time(last_database_update(), NULL, buf, sizeof(buf))) {
		snprintf(comment, sizeof(comment), "Not updated since %s", buf);
		print_status(title, comment, STATUS_STOPPED);
		teerank_stopped = 1;
	} else {
		print_status(title, NULL, STATUS_OK);
	}

	show_masters_status(teerank_stopped);
	html_footer(NULL, NULL);
}
