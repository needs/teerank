#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "info.h"

enum {
	STATUS_OK,
	STATUS_DOWN,
	STATUS_STOPPED,
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

	html("<section class=\"status status_%s\">", status_class(status));
	html("<span>%s</span>", title);
	if (comment)
		html("<span class=\"comment\">%s</span>", comment);
	html("<span>%s</span>", status_text(status));
	html("</section>");
}

int main_html_status(int argc, char **argv)
{
	const char *title;
	char buf[16], comment[64];
	struct info info;
	short teerank_stopped = 0;
	unsigned i;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	CUSTOM_TAB.name = "Status";
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, "Status", "", NULL);

	html("<h2>Teerank</h2>");

	/*
	 * Teerank status, since the whole status is in the info struct,
	 * failing to read it is fatal.
	 */
	if (!read_info(&info)) {
		print_status("Teerank", "Can't read last update date", STATUS_UNKNOWN);
		html_footer(NULL, NULL);
		return EXIT_SUCCESS;
	}

	if (!elapsed_time_since(&info.last_update, NULL, buf, sizeof(buf))) {
		snprintf(comment, sizeof(comment), "Not updated since %s", buf);
		print_status("Teerank", comment, STATUS_STOPPED);
		teerank_stopped = 1;
	} else {
		print_status("Teerank", NULL, STATUS_OK);
	}

	title = "Teerank 2.x backward compatibility";
	if (!ROUTE_V2_URLS)
		print_status(title, NULL, STATUS_DISABLED);
	else if (teerank_stopped)
		print_status(title, comment, STATUS_STOPPED);
	else
		print_status(title, NULL, STATUS_OK);

	if (info.nmasters)
		html("<h2>Teeworlds</h2>");

	for (i = 0; i < info.nmasters; i++) {
		struct master_info *minfo = &info.masters[i];

		/*
		 * If teerank is stopped, then we can't really guess
		 * masters status.
		 */
		if (teerank_stopped)
			print_status(minfo->node, NULL, STATUS_UNKNOWN);

		else if (minfo->lastseen == NEVER_SEEN)
			print_status(minfo->node, NULL, STATUS_DOWN);

		else if (!elapsed_time_since(gmtime(&minfo->lastseen), NULL, buf, sizeof(buf))) {
			snprintf(comment, sizeof(comment), "Since %s", buf);
			print_status(minfo->node, comment, STATUS_DOWN);

		} else {
			snprintf(comment, sizeof(comment), "%u servers", minfo->nservers);
			print_status(minfo->node, comment, STATUS_OK);
		}
	}

	html_footer(NULL, NULL);

	return EXIT_SUCCESS;
}
