#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#include "cgi.h"
#include "player.h"
#include "clan.h"
#include "server.h"
#include "teerank.h"
#include "html.h"

static void sputc(char **buf, int *n, char c)
{
	if (*n) {
		**buf = c;
		*buf = *buf + 1;
		*n = *n - 1;
	}
}

static void sputs(char **buf, int *n, const char *str)
{
	while (*n && *str) {
		sputc(buf, n, *str);
		str++;
	}
}

static void sprint(char **buf, int *n, const char *fmt, ...)
{
	char tmp[16];
	va_list ap;
	int ret;

	/*
	 * We don't put the result directly in *buf because we don't
	 * want the ending nul character.
	 */
	va_start(ap, fmt);
	ret = vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);

	if (ret >= sizeof(tmp))
		ret = sizeof(tmp) - 1;
	if (ret > *n)
		ret = *n;

	memcpy(*buf, tmp, ret);

	*n = *n - ret;
	*buf = *buf + ret;
}

typedef void (*escape_func_t)(char **buf, int *n, const char *str);

static int print(char *buf, int size, escape_func_t escape, const char *fmt, va_list ap)
{
	int n = size;

	for (; *fmt && n; fmt++) {
		if (*fmt != '%') {
			sputc(&buf, &n, *fmt);
			continue;
		}

		switch (*++fmt) {
		case 's':
			escape(&buf, &n, va_arg(ap, char *));
			break;
		case 'S':
			sputs(&buf, &n, va_arg(ap, char *));
			break;
		case 'i':
			sprint(&buf, &n, "%i", va_arg(ap, int));
			break;
		case 'I':
			sprint(&buf, &n, "%li", va_arg(ap, long));
			break;
		case 'u':
			sprint(&buf, &n, "%u", va_arg(ap, unsigned));
			break;
		case 'U':
			sprint(&buf, &n, "%lu", va_arg(ap, long unsigned));
			break;
		case 'f':
			sprint(&buf, &n, "%.1f", va_arg(ap, double));
			break;
		case 'c':
			sputc(&buf, &n, va_arg(ap, int));
			break;
		case '%':
			sputc(&buf, &n, '%');
			break;
		default:
			fprintf(stderr, "Unknown conversion specifier '%%%c'\n", *fmt);
			abort();
		}
	}

	return size - n;
}

static void escape_xml(char **buf, int *n, const char *str)
{
	for (; *str && *n; str++) {
		switch (*str) {
		case '&':
			sputs(buf, n, "&amp;");
			break;
		case '<':
			sputs(buf, n, "&lt;");
			break;
		case '>':
			sputs(buf, n, "&gt;");
			break;
		case '\"':
			sputs(buf, n, "&quot;");
			break;
		case '\'':
			sputs(buf, n, "&#39;");
			break;
		default:
			sputc(buf, n, *str);
			break;
		}
	}
}

/* Store the result in a static buffer and print it right away */
static void _print(escape_func_t escape, const char *fmt, va_list ap)
{
	static char buf[1024];
	int ret = print(buf, sizeof(buf), escape, fmt, ap);
	fwrite(buf, ret, 1, stdout);
}

void html(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_print(escape_xml, fmt, ap);
	va_end(ap);
}

void xml(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_print(escape_xml, fmt, ap);
	va_end(ap);
}

void svg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_print(escape_xml, fmt, ap);
	va_end(ap);
}

void css(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_print(sputs, fmt, ap);
	va_end(ap);
}

static char dectohex(char dec)
{
	switch (dec) {
	case 0: return '0';
	case 1: return '1';
	case 2: return '2';
	case 3: return '3';
	case 4: return '4';
	case 5: return '5';
	case 6: return '6';
	case 7: return '7';
	case 8: return '8';
	case 9: return '9';
	case 10: return 'A';
	case 11: return 'B';
	case 12: return 'C';
	case 13: return 'D';
	case 14: return 'E';
	case 15: return 'F';
	default: return '0';
	}
}

/*
 * We only let through alpha-numeric characters as well as '-', '_', '.'
 * and '~'.  Every other characters must be encoded.
 */
static bool safe_url_char(char c)
{
	if (isalnum(c))
		return 1;

	return strchr("-_.~", c) != NULL;
}

static void url_encode(char **buf, int *n, const char *str)
{
	for (; *str && *n; str++) {
		if (safe_url_char(*str)) {
			sputc(buf, n, *str);
		} else {
			unsigned char c = *(unsigned char*)str;
			sputc(buf, n, '%');
			sputc(buf, n, dectohex(c / 16));
			sputc(buf, n, dectohex(c % 16));
		}
	}
}

#define NBUF 4
#define BUFSIZE 1024

char *URL(const char *fmt, ...)
{
	static char bufs[NBUF][BUFSIZE];
	static int i = 0;

	char *buf;
	va_list ap;
	int ret;

	/*
	 * Rotates buffers so this function can be called multiple time
	 * in a row and will returns different buffers.
	 */
	buf = bufs[i];
	i = (i + 1) % NBUF;

	/* Reserve one byte for the final '\0' */
	va_start(ap, fmt);
	ret = print(buf, BUFSIZE-1, url_encode, fmt, ap);
	va_end(ap);

	buf[ret] = '\0';
	return buf;
}

#undef BUFSIZE
#undef NBUF

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

void player_lastseen_link(time_t lastseen, const char *addr)
{
	char text[64], strls[] = "00/00/1970 00h00", *timescale;
	int is_online, have_strls;

	if (lastseen == NEVER_SEEN)
		return;

	is_online = !elapsed_time(lastseen, &timescale, text, sizeof(text));
	have_strls = strftime(strls, sizeof(strls), "%d/%m/%Y %Hh%M", gmtime(&lastseen));

	if (is_online && have_strls)
		html("<a class=\"%s\" href=\"/server/%s\" title=\"%s\">%s</a>",
		     timescale, addr, strls, text);

	else if (is_online)
		html("<a class=\"%s\" href=\"/server/%s\">%s</a>",
		     timescale, addr, text);

	else if (have_strls)
		html("<span class=\"%s\" title=\"%s\">%s</span>",
		     timescale, strls, text);

	else
		html("<span class=\"%s\">%s</span>", timescale, text);
}

struct tab CTF_TAB = { "CTF", "/players?gametype=CTF" };
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

struct colinfo {
	char *name;
	char *order;
};

static void html_list_header(
	char *class, char *listurl, unsigned pnum, char *order, struct colinfo *col)
{
	const char *down = "<img src=\"/images/downarrow.png\"/>";
	const char *dash = "<img src=\"/images/dash.png\"/>";

	html("<table class=\"%s\">", class);
	html("<thead>");
	html("<tr>");

	for (; col->name; col++) {
		if (!col->order || !order)
			html("<th>%s</th>", col->name);
		else if (strcmp(order, col->order) == 0)
			html("<th>%s%S</th>", col->name, down);
		else
			html("<th><a href=\"%S&sort=%S&p=%u\">%s%S</a></th>",
			     listurl, col->order, pnum, col->name, dash);
	}

	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

static void html_list_footer(void)
{
	html("</tbody>");
	html("</table>");
}

void html_start_player_list(char *listurl, unsigned pnum, char *order)
{
	struct colinfo colinfo[] = {
		{ "" },
		{ "Name" },
		{ "Clan" },
		{ "Elo", "rank" },
		{ "Last seen", "lastseen" },
		{ NULL }
	};

	html_list_header("playerlist", listurl, pnum, order, colinfo);
}

void html_start_online_player_list(void)
{
	struct colinfo colinfo[] = {
		{ "" },
		{ "Name" },
		{ "Clan" },
		{ "Score" },
		{ "Elo" },
		{ "Last seen" },
		{ NULL }
	};

	html_list_header("playerlist", "", 0, "", colinfo);
}

void html_end_player_list(void)
{
	html_list_footer();
}

void html_end_online_player_list(void)
{
	html_list_footer();
}

static void player_list_entry(
	struct player *p, struct client *c, int no_clan_link)
{
	const char *specimg = "<img src=\"/images/spectator.png\" title=\"Spectator\"/>";
	char *name, *clan;
	int spectator;

	assert(p || c);

	/* Spectators are less important */
	spectator = c && !c->ingame;
	if (spectator)
		html("<tr class=\"spectator\">");
	else
		html("<tr>");

	/* Rank */
	if (p && p->rank != UNRANKED)
		html("<td>%u</td>", p->rank);
	else
		html("<td title=\"Will be calculated within the next minutes\">...</td>");

	/* Name */
	name = p ? p->name : c->name;
	html("<td>%s<a href=\"%S\">%s</a></td>",
	     spectator ? specimg : "", URL("/player?name=%s", name), name);

	/* Clan */
	clan = p ? p->clan : c->clan;
	if (no_clan_link || !clan[0])
		html("<td>%s</td>", clan);
	else
		html("<td><a href=\"%S\">%s</a></td>", URL("/clan?name=%s", clan), clan);

	/* Score (online-player-list only) */
	if (c)
		html("<td>%i</td>", c->score);

	/* Elo */
	if (p)
		html("<td>%i</td>", p->elo);
	else
		html("<td>?</td>");

	/* Last seen (not online-player-list only) */
	html("<td>");
	if (p && !c)
		player_lastseen_link(p->lastseen, build_addr(p->server_ip, p->server_port));
	html("</td>");

	html("</tr>");
}

void html_player_list_entry(
	struct player *p, struct client *c, int no_clan_link)
{
	player_list_entry(p, c, no_clan_link);
}

void html_online_player_list_entry(struct player *p, struct client *c)
{
	player_list_entry(p, c, 0);
}

void html_start_clan_list(char *listurl, unsigned pnum, char *order)
{
	struct colinfo colinfo[] = {
		{ "" },
		{ "Name" },
		{ "Members" },
		{ NULL }
	};

	html_list_header("clanlist", listurl, pnum, order, colinfo);
}

void html_end_clan_list(void)
{
	html_list_footer();
}

void html_clan_list_entry(
	unsigned pos, const char *name, unsigned nmembers)
{
	assert(name != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	html("<td><a href=\"%S\">%s</a></td>", URL("/clan?name=%s", name), name);

	/* Members */
	html("<td>%u</td>", nmembers);

	html("</tr>");
}

void html_start_server_list(char *listurl, unsigned pnum, char *order)
{
	struct colinfo colinfo[] = {
		{ "" },
		{ "Name" },
		{ "Gametype" },
		{ "Map" },
		{ "Players" },
		{ NULL }
	};

	html_list_header("serverlist", listurl, pnum, order, colinfo);
}

void html_end_server_list(void)
{
	html_list_footer();
}

void html_server_list_entry(unsigned pos, struct server *server)
{
	assert(server != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	html("<td><a href=\"%S\">%s</a></td>",
	     URL("/server?addr=%s", build_addr(server->ip, server->port)), server->name);

	/* Gametype */
	html("<td>%s</td>", server->gametype);

	/* Map */
	html("<td>%s</td>", server->map);

	/* Players */
	html("<td>%u / %u</td>", server->num_clients, server->max_clients);

	html("</tr>");
}

/*
 * Only keep the two most significant digits
 */
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

void print_section_tabs(enum section_tab tab, const char *format, unsigned *tabvals)
{
	unsigned i;

	struct {
		const char *title;
		const char *url;
	} tabs[] = {
		{ "Players", "players" },
		{ "Clans",   "clans" },
		{ "Servers", "servers" }
	};

	assert(tabvals != NULL);
	assert(format != NULL);

	tabvals[0] = round(tabvals[0]);
	tabvals[1] = round(tabvals[1]);
	tabvals[2] = round(tabvals[2]);

	html("<nav class=\"section_tabs\">");

	for (i = 0; i < SECTION_TABS_COUNT; i++) {
		char url[128];
		snprintf(url, sizeof(url), format, tabs[i].url);

		if (i == tab)
			html("<a class=\"enabled\">");
		else
			html("<a href=\"%S\">", url);

		html("%s", tabs[i].title);
		if (tabvals[i])
			html("<small>%u</small>", tabvals[i]);

		html("</a>");
	}

	html("</nav>");
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

static char *url(const char *urlfmt, unsigned pnum)
{
	static char ret[128];
	snprintf(ret, sizeof(ret), urlfmt, pnum);
	return ret;
}

void print_page_nav(const char *fmt, unsigned pnum, unsigned npages)
{
	/* Number of pages shown before and after the current page */
	static const unsigned extra = 3;
	unsigned i;

	assert(fmt != NULL);

	html("<nav class=\"pages\">");

	/* Previous button */
	if (pnum == 1)
		html("<a class=\"previous\">Previous</a>");
	else
		html("<a class=\"previous\" href=\"%S\">Previous</a>", url(fmt, pnum-1));

	/* Link to first page */
	if (pnum > extra + 1)
		html("<a href=\"%S\">1</a>", url(fmt, 1));
	if (pnum > extra + 2)
		html("<span>...</span>");

	/* Extra pages before */
	for (i = min(extra, pnum - 1); i > 0; i--)
		html("<a href=\"%S\">%u</a>", url(fmt, pnum-i), pnum - i);

	html("<a class=\"current\">%u</a>", pnum);

	/* Extra pages after */
	for (i = 1; i <= min(extra, npages - pnum); i++)
		html("<a href=\"%S\">%u</a>", url(fmt, pnum+i), pnum + i);

	/* Link to last page */
	if (pnum + extra + 1 < npages)
		html("<span>...</span>");
	if (pnum + extra < npages)
		html("<a href=\"%S\">%u</a>", url(fmt, npages), npages);

	/* Next button */
	if (pnum == npages)
		html("<a class=\"next\">Next</a>");
	else
		html("<a class=\"next\" href=\"%S\">Next</a>", url(fmt, pnum+1));

	html("</nav>");
}
