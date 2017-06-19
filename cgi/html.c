#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include "cgi.h"
#include "clan.h"
#include "server.h"
#include "teerank.h"
#include "html.h"

/* Minutes looks nice when they only ends by '0' or '5' */
static unsigned pretiffy_minutes(unsigned minutes)
{
	return minutes - (minutes % 5);
}

/*
 * Make a raw evaluation of the elapsed time in years, months, days,
 * hours, minutes and secondes.  Doesn't try to be hyper accurate, but
 * rather give a good enough approximation.
 *
 * Returns a struct tm only to reuse it's fields, it is not by any means
 * a valid, conventional struct tm usable in any standard functions.
 */
static struct tm broke_down_elapsed_time(time_t sec)
{
	const time_t MINUTE = 60;
	const time_t HOUR = 60 * MINUTE;
	const time_t DAY = 24 * HOUR;
	const time_t MONTH = 30.4 * DAY;
	const time_t YEAR = 12 * MONTH;

	struct tm ret;

	if ((ret.tm_year = sec / YEAR))
		sec -= ret.tm_year * YEAR;
	if ((ret.tm_mon = sec / MONTH))
		sec -= ret.tm_mon * MONTH;
	if ((ret.tm_mday = sec / DAY))
		sec -= ret.tm_mday * DAY;
	if ((ret.tm_hour = sec / HOUR))
		sec -= ret.tm_hour * HOUR;
	if ((ret.tm_min = sec / MINUTE))
		sec -= ret.tm_min * MINUTE;
	if ((ret.tm_sec = sec))
		sec -= ret.tm_sec;

	assert(ret.tm_mon < 12);
	assert(ret.tm_mday < 31);
	assert(ret.tm_hour < 24);
	assert(ret.tm_min < 60);
	assert(ret.tm_sec < 60);

	return ret;
}

/*
 * Use the given timescale and elapsed time to build a string, and take
 * care of the eventual plural form.
 */
#define set_text_and_timescale(ts, val) do { \
	*timescale = ts "s"; \
	if (text) \
		snprintf(text, textsize, "%d " ts "%s", (val), (val) > 1 ? "s" : ""); \
} while (0)

unsigned elapsed_time(time_t t, char **timescale, char *text, size_t textsize)
{
	time_t now = time(NULL);
	time_t elapsed_seconds;
	struct tm elapsed;
	char *dummy;

	if (!timescale)
		timescale = &dummy;

	/* Make sure elapsed time is positive */
	if (now < t)
		elapsed_seconds = t - now;
	else
		elapsed_seconds = now - t;

	elapsed = broke_down_elapsed_time(elapsed_seconds);

	if (elapsed.tm_year) {
		set_text_and_timescale("year", elapsed.tm_year);
		return elapsed.tm_year;
	}
	if (elapsed.tm_mon) {
		set_text_and_timescale("month", elapsed.tm_mon);
		return elapsed.tm_mon;
	}
	if (elapsed.tm_mday) {
		set_text_and_timescale("day", elapsed.tm_mday);
		return elapsed.tm_mday;
	}
	if (elapsed.tm_hour) {
		set_text_and_timescale("hour", elapsed.tm_hour);
		return elapsed.tm_hour;
	}
	if (elapsed.tm_min >= 5) {
		set_text_and_timescale("minute", pretiffy_minutes(elapsed.tm_min));
		return elapsed.tm_min;
	}

	snprintf(text, textsize, "Online");
	*timescale = "online";
	return 0;
}

#undef set_text_and_timescale

void player_lastseen_link(time_t lastseen, char *ip, char *port)
{
	char text[64], strls[] = "00/00/1970 00h00", *timescale;
	int is_online, have_strls;
	url_t url;

	if (!lastseen)
		return;

	is_online = !elapsed_time(lastseen, &timescale, text, sizeof(text));
	have_strls = strftime(strls, sizeof(strls), "%d/%m/%Y %Hh%M", gmtime(&lastseen));
	URL(url, "/server", PARAM_IP(ip), PARAM_PORT(port));

	if (is_online && have_strls)
		html("<a class=\"%s\" href=\"%S\" title=\"%s\">%s</a>",
		     timescale, url, strls, text);

	else if (is_online)
		html("<a class=\"%s\" href=\"%S\">%s</a>",
		     timescale, url, text);

	else if (have_strls)
		html("<span class=\"%s\" title=\"%s\">%s</span>",
		     timescale, strls, text);

	else
		html("<span class=\"%s\">%s</span>", timescale, text);
}

struct tab CTF_TAB = { "CTF", "/players" };
struct tab DM_TAB  = { "DM",  "/players?gametype=DM"  };
struct tab TDM_TAB = { "TDM", "/players?gametype=TDM" };
struct tab ABOUT_TAB = { "About", "/about" };

static void print_top_tabs(void *active)
{
	struct tab **tabs;
	struct tab CUSTOM_TAB;

	assert(active != NULL);

	html("<nav id=\"toptabs\">");

	/*
	 * If "active" don't point to one of the fixed tabs, we add an
	 * extra tab before the about tab and use the data pointed by
	 * "active" as the tab name.
	 */
	if (
		active == &CTF_TAB ||
		active == &DM_TAB ||
		active == &TDM_TAB ||
		active == &ABOUT_TAB) {

		tabs = (struct tab *[]){
			&CTF_TAB, &DM_TAB, &TDM_TAB, &ABOUT_TAB, NULL
		};
	} else {
		CUSTOM_TAB.name = active;
		tabs = (struct tab *[]){
			&CTF_TAB, &DM_TAB, &TDM_TAB, &CUSTOM_TAB, &ABOUT_TAB, NULL
		};
		active = &CUSTOM_TAB;
	}

	for (; *tabs; tabs++) {
		const struct tab *tab = *tabs;
		char *class = "";

		if (tab == active && tab == &CUSTOM_TAB)
			class = " class=\"active custom\"";
		else if (tab == active && tab != &CUSTOM_TAB)
			class = " class=\"active\"";

		if (tab == active)
			html("<a%S>%s</a>", class, tab->name);
		else
			html("<a href=\"%s\"%S>%s</a>",
			       tab->href, class, tab->name);
	}

	html("</nav>");
}

void html_header(
	void *active, const char *title,
	const char *sprefix, const char *query)
{
	char text[64];

	assert(title != NULL);
	assert(sprefix != NULL);

	html("<!doctype html>");
	html("<html>");
	html("<head>");
	html("<meta charset=\"utf-8\"/>");
	html("<title>%s - Teerank</title>", title);

	html("<meta name=\"description\" content=\"Teerank is a simple and fast ranking system for teeworlds.\"/>");
	html("<link rel=\"stylesheet\" href=\"/style.css\"/>");
	html("</head>");
	html("<body>");
	html("<header>");
	html("<a id=\"logo\" href=\"/\"><img src=\"/images/logo.png\" alt=\"Logo\"/></a>");

	html("<section>");

	/*
	 * Show a warning banner if the database has not been updated
	 * since 10 minutes.
	 */
	if (elapsed_time(last_database_update(), NULL, text, sizeof(text)))
		html("<a id=\"alert\" href=\"/status\">Not updated since %s</a>", text);

	html("<form action=\"search%S\" id=\"searchform\">", sprefix);
	html("<input name=\"q\" type=\"text\" placeholder=\"Search\"");
	if (query)
		html(" value=\"%s\"", query);
	html("/>");

	html("<input type=\"submit\" value=\"\"/>");
	html("</form>");
	html("</section>");

	html("</header>");
	html("<main>");
	print_top_tabs(active);
	html("<section>");
}

void html_footer(const char *jsonanchor, const char *jsonurl)
{
	assert((jsonanchor && jsonurl) || (!jsonanchor && !jsonurl));

	html("</section>");

	if (jsonanchor && jsonurl) {
		html("<nav id=\"bottabs\">");
		html("<a href=\"%S\">JSON</a>", jsonurl);
		html("<a href=\"/about-json-api#%S\">JSON Doc</a>", jsonanchor);
		html("<a class=\"active\">HTML</a>");
		html("</nav>");
	}

	html("</main>");

	html("<footer id=\"footer\">");
	html("<ul>");
	html("<li>");
	html("<a href=\"https://github.com/needs/teerank/commit/%S\">Teerank %i.%i</a>",
	     CURRENT_COMMIT, TEERANK_VERSION, TEERANK_SUBVERSION);
	html(" ");
	html("<a href=\"https://github.com/needs/teerank/tree/%S\">(%s)</a>",
	     CURRENT_BRANCH, STABLE_VERSION ? "stable" : "unstable");
	html("</li>");
	html("<li><a href=\"/status\">Status</a></li>");
	html("<li><a href=\"/about\">About</a></li>");
	html("</ul>");
	html("</footer>");

	html("</body>");
	html("</html>");
}

static void list_header(
	struct html_list_column *cols, char *order, list_class_func_t class_cb,
	url_t listurl, unsigned pnum)
{
	const char *down = "<img src=\"/images/downarrow.png\"/>";
	const char *dash = "<img src=\"/images/dash.png\"/>";
	struct html_list_column *col;
	char *class = NULL;
	url_t url;

	if (class_cb)
		class = class_cb(NULL);
	if (class)
		html("<table class=\"%s\">", class);
	else
		html("<table>");

	html("<thead>");
	html("<tr>");

	for (col = cols; col->title; col++) {
		if (!col->order || !order || !listurl)
			html("<th>%s</th>", col->title);
		else if (strcmp(order, col->order) == 0)
			html("<th>%s%S</th>", col->title, down);
		else {
			URL(url, listurl, PARAM_ORDER(col->order), PARAM_PAGENUM(pnum));
			html("<th><a href=\"%S\">%s%S</a></th>",
			     url, col->title, dash);
		}
	}

	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

static void list_footer(url_t fmt, bool empty, unsigned pnum, unsigned nrow)
{
	/* Number of pages shown before and after the current page */
	static const unsigned extra = 3;
	unsigned i, npages;
	url_t url;

	html("</tbody>");
	html("</table>");

	if (empty)
		html("<em class=\"no-results\">No results found</em>");

	if (!fmt)
		return;

	npages = nrow / 100 + 1;

	html("<nav class=\"pages\">");

	/* Previous button */
	if (pnum == 1)
		html("<a class=\"previous\">Previous</a>");
	else {
		URL(url, fmt, PARAM_PAGENUM(pnum - 1));
		html("<a class=\"previous\" href=\"%S\">Previous</a>", url);
	}

	/* Link to first page */
	if (pnum > extra + 1) {
		URL(url, fmt, PARAM_PAGENUM(1));
		html("<a href=\"%S\">1</a>", url);
	}
	if (pnum > extra + 2)
		html("<span>...</span>");

	/* Extra pages before */
	for (i = min(extra, pnum - 1); i > 0; i--) {
		URL(url, fmt, PARAM_PAGENUM(pnum - i));
		html("<a href=\"%S\">%u</a>", url, pnum - i);
	}

	html("<a class=\"current\">%u</a>", pnum);

	/* Extra pages after */
	for (i = 1; i <= min(extra, npages - pnum); i++) {
		URL(url, fmt, PARAM_PAGENUM(pnum + i));
		html("<a href=\"%S\">%u</a>", url, pnum + i);
	}

	/* Link to last page */
	if (pnum + extra + 1 < npages)
		html("<span>...</span>");
	if (pnum + extra < npages) {
		URL(url, fmt, PARAM_PAGENUM(npages));
		html("<a href=\"%S\">%u</a>", url, npages);
	}

	/* Next button */
	if (pnum == npages)
		html("<a class=\"next\">Next</a>");
	else {
		URL(url, fmt, PARAM_PAGENUM(pnum + 1));
		html("<a class=\"next\" href=\"%S\">Next</a>", url);
	}

	html("</nav>");
}

struct list_args {
	struct html_list_column *cols;
	list_class_func_t class_cb;
};

#define next(stype, ctype) (ctype)sqlite3_column_##stype(res, i++)
#define is_null(i) sqlite3_column_type(res, i)

static void list_item(sqlite3_stmt *res, void *args_)
{
	struct list_args *args = args_;
	struct html_list_column *col = args->cols;
	char *class = NULL;
	unsigned i = 0;
	url_t url;

	if (args->class_cb)
		class = args->class_cb(res);
	if (class)
		html("<tr class=\"%s\">", class);
	else
		html("<tr>");

	for (col = args->cols; col->title; col++) {
		switch (col->type) {
		case HTML_COLTYPE_NONE: {
			char *val = next(text, char *);
			html("<td>%s</td>", val);
			break;
		}

		case HTML_COLTYPE_RANK: {
			unsigned rank = next(int64, unsigned);
			if (!is_null(i-1))
				html("<td>%u</td>", rank);
			else
				html("<td title=\"Will be calculated within the next minutes\">...</td>");
			break;
		}

		case HTML_COLTYPE_ELO: {
			int elo = next(int, int);
			if (!is_null(i-1))
				html("<td>%i</td>", elo);
			else
				html("<td title=\"Will be calculated within the next minutes\">...</td>");
			break;
		}

		case HTML_COLTYPE_PLAYER: {
			char *name = next(text, char *);
			URL(url, "/player", PARAM_NAME(name));
			html("<td><a href=\"%S\">%s</a></td>", url, name);
			break;
		}

		case HTML_COLTYPE_ONLINE_PLAYER: {
			char *name = next(text, char *);
			bool ingame = next(int, bool);
			char *specimg =
				"<img"
				" src=\"/images/spectator.png\""
				" title=\"Spectator\"/>";

			html("<td>");
			if (!ingame)
				html("%S", specimg);
			URL(url, "/player", PARAM_NAME(name));
			html("<a href=\"%S\">%s</a>", url, name);
			html("</td>");
			break;
		}

		case HTML_COLTYPE_CLAN: {
			char *clan = next(text, char *);
			URL(url, "/clan", PARAM_NAME(clan));
			html("<td><a href=\"%S\">%s</a></td>", url, clan);
			break;
		}

		case HTML_COLTYPE_SERVER: {
			char *name = next(text, char *);
			char *ip = next(text, char *);
			char *port = next(text, char *);

			URL(url, "/server", PARAM_IP(ip), PARAM_PORT(port));
			html("<td><a href=\"%S\">%s</a></td>", url, name);
			break;
		}

		case HTML_COLTYPE_LASTSEEN: {
			time_t lastseen = next(int64, time_t);
			char *ip = next(text, char *);
			char *port = next(text, char *);

			html("<td>");
			player_lastseen_link(lastseen, ip, port);
			html("</td>");
			break;
		}

		case HTML_COLTYPE_PLAYER_COUNT: {
			char *nplayers = next(text, char *);
			char *maxplayers = next(text, char *);
			html("<td>%s / %s</td>", nplayers, maxplayers);
			break;
		}
		}
	}
	html("</tr>");
}

#undef is_null
#undef next

void html_list(
	sqlite3_stmt *res, struct html_list_column *cols, char *order,
	list_class_func_t class, url_t url, unsigned pnum, unsigned nrow)
{
	struct list_args args = { cols, class };
	bool empty = true;

	list_header(cols, order, class, url, pnum);
	while (foreach_next(&res, &args, list_item))
		empty = false;
	list_footer(url, empty, pnum, nrow);
}

/* Only keep the two most significant digits */
static unsigned round(unsigned n)
{
	unsigned mod = 1;

	while (mod < n)
		mod *= 10;

	mod /= 100;

	if (mod == 0)
		return n;

	return n - (n % mod);
}

void print_section_tabs(struct section_tab *tabs)
{
	assert(tabs != NULL);

	html("<nav class=\"section_tabs\">");

	for (; tabs->title; tabs++) {
		if (tabs->active)
			html("<a class=\"enabled\">");
		else
			html("<a href=\"%S\">", tabs->url);

		html("%s", tabs->title);
		if (tabs->val)
			html("<small>%u</small>", round(tabs->val));

		html("</a>");
	}

	html("</nav>");
}
