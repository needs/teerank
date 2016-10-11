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

int page_status_html_main(int argc, char **argv)
{
	char buf[16], comment[64];
	struct info info;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Eventually, print them */
	CUSTOM_TAB.name = "Status";
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, "Status", NULL, NULL);

	html("<h2>Teerank</h2>");

	/* Teerank status */
	if (!read_info(&info)) {
		print_status("Teerank", "Can't read last update date", STATUS_UNKNOWN);
	} else if (!elapsed_time_since(&info.last_update, NULL, buf, sizeof(buf))) {
		snprintf(comment, sizeof(comment), "Not updated since %s", buf);
		print_status("Teerank", comment, STATUS_STOPPED);
	} else {
		print_status("Teerank", NULL, STATUS_OK);
	}

	print_status("JSON API", NULL, STATUS_DISABLED);

	html("<h2>Teeworlds</h2>");
	print_status("master1.teeworlds.com", NULL, STATUS_DOWN);
	print_status("master2.teeworlds.com", NULL, STATUS_OK);
	print_status("master3.teeworlds.com", NULL, STATUS_OK);
	print_status("master4.teeworlds.com", NULL, STATUS_UNKNOWN);

	html_footer(NULL, NULL);

	return EXIT_SUCCESS;
}
