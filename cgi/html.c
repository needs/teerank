#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include "cgi.h"
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

static void print_top_tab(
	char *title, url_t url, char *class, char *subtitle, url_t suburl)
{
	assert(title != NULL);

	if (class)
		html("<li class=\"%s\">", class);
	else
		html("<li>");

	if (url)
		html("<a href=\"%S\">", url);
	else
		html("<a>");

	html("%s</a>", title);

	if (subtitle && !suburl)
		html("<a>%s</a>", subtitle);
	else if (subtitle && suburl)
		html("<a href=\"%S\">%s</a>", suburl, subtitle);

	html("</li>");
}

static char *tab_title(enum tab_type tab, char *dflt)
{
	switch (tab) {
	case CTF_TAB:
		return "CTF";
	case DM_TAB:
		return "DM";
	case TDM_TAB:
		return "TDM";
	case ABOUT_TAB:
		return "About";
	default:
		return dflt;
	}
}

static void print_top_tabs(
	enum tab_type active, char *custom_title, char *subtitle, url_t suburl)
{
	url_t url;
	char *title;
	enum tab_type tab;
	html("<nav id=\"toptabs\">");

	for (tab = 0; tab <= ABOUT_TAB; tab++) {
		if (tab == CTF_TAB)
			html("<ul>");
		else if (tab == CUSTOM_TAB)
			html("</ul><ul>");

		title = tab_title(tab, custom_title);

		if (tab == active) {
			char *class;
			if (tab == CUSTOM_TAB)
				class = "active custom";
			else
				class = "active";
			print_top_tab(title, NULL, class, subtitle, suburl);
			continue;
		}

		switch (tab) {
		case CTF_TAB:
			URL(url, "/players", PARAM_GAMETYPE("CTF"));
			break;
		case DM_TAB:
			URL(url, "/players", PARAM_GAMETYPE("DM"));
			break;
		case TDM_TAB:
			URL(url, "/players", PARAM_GAMETYPE("TDM"));
			break;
		case ABOUT_TAB:
			URL(url, "/about");
			break;
		default:
			continue;
		}
		print_top_tab(title, url, NULL, NULL, NULL);
	}

	html("</ul></nav>");
}

void html_header_(enum tab_type active, struct html_header_args args)
{
	char text[64];
	char *search_url;
	char *title;

	title = tab_title(active, args.title);

	html("<!doctype html>");
	html("<html>");
	html("<head>");
	html("<meta charset=\"utf-8\"/>");

	html("<title>");
	if (title && args.subtab)
		html("%s - %s - Teerank", args.subtab, title);
	else if (title)
		html("%s - Teerank", title);
	else
		html("Teerank");
	html("</title>");

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

	if (args.search_url)
		search_url = args.search_url;
	else
		search_url = "/search";

	html("<form action=\"%S\" id=\"searchform\">", search_url);
	html("<input name=\"q\" type=\"text\" placeholder=\"Search\"");
	if (args.search_query)
		html(" value=\"%s\"", args.search_query);
	html("/>");

	html("<input type=\"submit\" value=\"\"/>");
	html("</form>");
	html("</section>");

	html("</header>");
	html("<main>");
	print_top_tabs(
		active, args.title, args.subtab, args.subtab_url);
	html("<section>");
}

void html_footer(char *jsonanchor, char *jsonurl)
{
	assert((jsonanchor && jsonurl) || (!jsonanchor && !jsonurl));

	html("</section>");

	if (jsonanchor && jsonurl) {
		html("<nav id=\"bottabs\"><ul>");
		print_top_tab("JSON", jsonurl, NULL, NULL, NULL);
		print_top_tab("JSON Doc", jsonanchor, NULL, NULL, NULL);
		print_top_tab("HTML", NULL, "active", NULL, NULL);
		html("</ul></nav>");
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

static void list_item(sqlite3_stmt *res, struct html_list_column *col, list_class_func_t class_cb)
{
	char *class = NULL;
	unsigned i = 0;
	url_t url;

	if (class_cb)
		class = class_cb(res);
	if (class)
		html("<tr class=\"%s\">", class);
	else
		html("<tr>");

	while (col->title) {
		switch (col->type) {
		case HTML_COLTYPE_NONE: {
			char *val = column_text(res, i++);
			html("<td>%s</td>", val);
			break;
		}

		case HTML_COLTYPE_RANK: {
			unsigned rank = column_unsigned(res, i++);
			if (!is_column_null(res, i-1))
				html("<td>%u</td>", rank);
			else
				html("<td title=\"Will be calculated within the next minutes\">...</td>");
			break;
		}

		case HTML_COLTYPE_ELO: {
			int elo = column_int(res, i++);
			if (!is_column_null(res, i-1))
				html("<td>%i</td>", elo);
			else
				html("<td title=\"Will be calculated within the next minutes\">...</td>");
			break;
		}

		case HTML_COLTYPE_PLAYER: {
			char *name = column_text(res, i++);
			URL(url, "/player", PARAM_NAME(name));
			html("<td><a href=\"%S\">%s</a></td>", url, name);
			break;
		}

		case HTML_COLTYPE_ONLINE_PLAYER: {
			char *name = column_text(res, i++);
			bool ingame = column_bool(res, i++);
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
			char *clan = column_text(res, i++);
			URL(url, "/clan", PARAM_NAME(clan));
			html("<td><a href=\"%S\">%s</a></td>", url, clan);
			break;
		}

		case HTML_COLTYPE_SERVER: {
			char *name = column_text(res, i++);
			char *ip = column_text(res, i++);
			char *port = column_text(res, i++);

			URL(url, "/server", PARAM_IP(ip), PARAM_PORT(port));
			html("<td><a href=\"%S\">%s</a></td>", url, name);
			break;
		}

		case HTML_COLTYPE_LASTSEEN: {
			time_t lastseen = column_time_t(res, i++);
			char *ip = column_text(res, i++);
			char *port = column_text(res, i++);

			html("<td>");
			player_lastseen_link(lastseen, ip, port);
			html("</td>");
			break;
		}

		case HTML_COLTYPE_PLAYER_COUNT: {
			char *nplayers = column_text(res, i++);
			char *maxplayers = column_text(res, i++);
			html("<td>%s / %s</td>", nplayers, maxplayers);
			break;
		}
		}
		col++;
	}
	html("</tr>");
}

void html_list(
	sqlite3_stmt *res, struct html_list_column *cols, char *order,
	list_class_func_t class, url_t url, unsigned pnum, unsigned nrow)
{
	bool empty = true;

	list_header(cols, order, class, url, pnum);
	while ((res = foreach_next(res))) {
		list_item(res, cols, class);
		empty = false;
	}
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
