#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "cgi.h"
#include "player.h"
#include "clan.h"
#include "server.h"
#include "teerank.h"
#include "html.h"

#ifdef NDEBUG
void html(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void xml(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void svg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void css(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

#else  /* NDEBUG */

/*
 * Indentation level is shared so that nested languages (CSS in HTML
 * for instance) are not mis-indented.
 */
static int indent = 0;

static void print(const char *fmt, va_list ap)
{
	unsigned i;

	for (i = 0; i < indent; i++)
		putchar('\t');

	vprintf(fmt, ap);
	putchar('\n');
}

static void _xml(const char *fmt, va_list ap)
{
	int opening_tag = 0, closing_tag = 0;
	size_t len = strlen(fmt);

	if (*fmt == '\0')
		goto print;

	/* It it start with "<" then we have an opening_tag */
	if (fmt[0] == '<' && fmt[1] != '/' && fmt[1] != '!' && fmt[1] != '?')
		opening_tag = 1;

	/*
	 * If it end with "/>" or if the last tag start with "</" then
	 * we have a closing tag.
	 */
	if (fmt[len - 1] == '>' && fmt[len - 2] == '/') {
		closing_tag = 1;
	} else {
		const char *s = &fmt[len];

		while (s != fmt && *s != '<')
			s--;

		if (*s == '<' && *(s + 1) == '/')
			closing_tag = 1;
	}

print:
	if (!opening_tag && closing_tag) {
		assert(indent > 0);
		indent--;
	}

	print(fmt, ap);

	if (opening_tag && !closing_tag)
		indent++;
}

void html(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_xml(fmt, ap);
	va_end(ap);
}

void xml(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_xml(fmt, ap);
	va_end(ap);
}

void svg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_xml(fmt, ap);
	va_end(ap);
}

void css(const char *fmt, ...)
{
	va_list ap;

	if (fmt[0] == '}')
		indent--;

	va_start(ap, fmt);
	print(fmt, ap);
	va_end(ap);

	if (fmt[strlen(fmt) - 1] == '{')
		indent++;
}

#endif  /* NDEBUG */

const struct tab CTF_TAB = { "CTF", "/" };
const struct tab ABOUT_TAB = { "About", "/about" };
struct tab CUSTOM_TAB = { NULL, NULL };

char *escape(const char *str)
{
	static char buf[1024];
	char *ret = buf;

	assert(str != NULL);
	assert(strlen(str) <= sizeof(buf));

	for (; *str; str++) {
		switch (*str) {
		case '<':
			ret = stpcpy(ret, "&lt;"); break;
		case '>':
			ret = stpcpy(ret, "&gt;"); break;
		case '&':
			ret = stpcpy(ret, "&amp;"); break;
		case '"':
			ret = stpcpy(ret, "&quot;"); break;
		default:
			*ret++ = *str;
		}
	}

	*ret = '\0';
	return buf;
}

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

void html_header(
	const struct tab *active, const char *title,
	const char *sprefix, const char *query)
{
	char text[64];

	assert(active != NULL);
	assert(title != NULL);
	assert(sprefix != NULL);

	if (active == &CUSTOM_TAB) {
		assert(active->name != NULL);
		assert(active->href != NULL);
	}

	html("<!doctype html>");
	html("<html>");
	html("<head>");
	html("<meta charset=\"utf-8\"/>");
	html("<title>%s - Teerank</title>", escape(title));

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

	html("<form action=\"%s/search\" id=\"searchform\">", sprefix);
	html("<input name=\"q\" type=\"text\" placeholder=\"Search\"%s%s%s/>",
	     query ? " value=\"" : "",
	     query ? escape(query) : "",
	     query ? "\"" : "");

	html("<input type=\"submit\" value=\"\"/>");
	html("</form>");
	html("</section>");

	html("</header>");
	html("<main>");
	html("<nav id=\"toptabs\">");

	const struct tab **tabs = (const struct tab*[]){
		&CTF_TAB, &CUSTOM_TAB, &ABOUT_TAB, NULL
	};

	for (; *tabs; tabs++) {
		const struct tab *tab = *tabs;
		char *class = "";

		if (tab == &CUSTOM_TAB && tab != active)
			continue;

		if (tab == active && active == &CUSTOM_TAB)
			class = " class=\"active custom\"";
		else if (tab == active && active != &CUSTOM_TAB)
			class = " class=\"active\"";

		if (tab == active)
			html("<a%s>%s</a>", class, tab->name);
		else
			html("<a href=\"%s\"%s>%s</a>",
			       tab->href, class, tab->name);
	}

	html("</nav>");
	html("<section>");
}

void html_footer(const char *jsonanchor, const char *jsonurl)
{
	assert((jsonanchor && jsonurl) || (!jsonanchor && !jsonurl));

	html("</section>");

	if (jsonanchor && jsonurl) {
		html("<nav id=\"bottabs\">");
		html("<a href=\"%s\">JSON</a>", jsonurl);
		html("<a href=\"/about-json-api#%s\">JSON Doc</a>", jsonanchor);
		html("<a class=\"active\">HTML</a>");
		html("</nav>");
	}

	html("</main>");

	html("<footer id=\"footer\">");
	html("<ul>");
	html("<li>");
	html("<a href=\"https://github.com/needs/teerank/commit/%s\">Teerank %d.%d</a>",
	     CURRENT_COMMIT, TEERANK_VERSION, TEERANK_SUBVERSION);
	html(" ");
	html("<a href=\"https://github.com/needs/teerank/tree/%s\">(%s)</a>",
	     CURRENT_BRANCH, STABLE_VERSION ? "stable" : "unstable");
	html("</li>");
	html("<li><a href=\"/status\">Status</a></li>");
	html("<li><a href=\"/about\">About</a></li>");
	html("</ul>");
	html("</footer>");

	html("</body>");
	html("</html>");
}

static void start_player_list(
	int onlinelist, int byrank, int bylastseen, unsigned pnum)
{
	const char *selected = "<img src=\"/images/downarrow.png\"/>";
	const char *unselected = "<img src=\"/images/dash.png\"/>";

	assert(onlinelist || byrank || bylastseen);

	if (byrank && bylastseen)
		selected = unselected = "";

	html("<table class=\"playerlist\">");
	html("<thead>");
	html("<tr>");
	html("<th></th>");
	html("<th>Name</th>");
	html("<th>Clan</th>");

	/* Online player also have a score */
	if (onlinelist)
		html("<th>Score</th>");

	if (onlinelist)
		html("<th>Elo</th>");
	else if (byrank)
		html("<th>Elo%s</th>", selected);
	else
		html("<th><a href=\"/players?p=%u\">Elo%s</a></th>",
		     pnum, unselected);

	/*
	 * No need to display the last seen date if all players on the
	 * list are expected to be online
	 */
	if (!onlinelist) {
		if (bylastseen)
			html("<th>Last seen%s</th>", selected);
		else
			html("<th><a href=\"/players/by-lastseen?p=%u\">Last seen%s</a></th>",
			     pnum, unselected);
	}

	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

void html_start_player_list(int byrank, int bylastseen, unsigned pnum)
{
	start_player_list(0, byrank, bylastseen, pnum);
}

void html_start_online_player_list(void)
{
	start_player_list(1, 0, 0, 0);
}

void html_end_player_list(void)
{
	html("</tbody>");
	html("</table>");
}

void html_end_online_player_list(void)
{
	html("</tbody>");
	html("</table>");
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
	html("<td>%s<a href=\"/player/%s\">%s</a></td>",
	     spectator ? specimg : "", url_encode(name), escape(name));

	/* Clan */
	clan = p ? p->clan : c->clan;
	if (no_clan_link || !clan[0])
		html("<td>%s</td>", escape(clan));
	else
		html("<td><a href=\"/clan/%s\">%s</a></td>",
		     url_encode(clan), escape(clan));

	/* Score (online-player-list only) */
	if (c)
		html("<td>%d</td>", c->score);

	/* Elo */
	if (p)
		html("<td>%d</td>", p->elo);
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

void html_start_clan_list(void)
{
	html("<table class=\"clanlist\">");
	html("<thead>");
	html("<tr>");
	html("<th></th>");
	html("<th>Name</th>");
	html("<th>Members</th>");
	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

void html_end_clan_list(void)
{
	html("</tbody>");
	html("</table>");
}

void html_clan_list_entry(
	unsigned pos, const char *name, unsigned nmembers)
{
	assert(name != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	html("<td><a href=\"/clan/%s\">%s</a></td>", url_encode(name), escape(name));

	/* Members */
	html("<td>%u</td>", nmembers);

	html("</tr>");
}

void html_start_server_list(void)
{
	html("<table class=\"serverlist\">");
	html("<thead>");
	html("<tr>");
	html("<th></th>");
	html("<th>Name</th>");
	html("<th>Gametype</th>");
	html("<th>Map</th>");
	html("<th>Players</th>");
	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

void html_end_server_list(void)
{
	html("</tbody>");
	html("</table>");
}

void html_server_list_entry(unsigned pos, struct server *server)
{
	assert(server != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	html("<td><a href=\"/server/%s\">%s</a></td>",
	     build_addr(server->ip, server->port), escape(server->name));

	/* Gametype */
	html("<td>%s</td>", escape(server->gametype));

	/* Map */
	html("<td>%s</td>", escape(server->map));

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

void print_section_tabs(enum section_tab tab, const char *squery, unsigned *tabvals)
{
	unsigned i;

	struct {
		const char *title;
		const char *url;
		unsigned num;
	} default_tabs[] = {
		{ "Players", "/players", 0 },
		{ "Clans", "/clans", 0 },
		{ "Servers", "/servers", 0 }
	}, search_tabs[] = {
		{ "Players", "/players/search", 0 },
		{ "Clans", "/clans/search", 0 },
		{ "Servers", "/servers/search", 0 }
	}, *tabs;

	if (squery)
		tabs = search_tabs;
	else
		tabs = default_tabs;

	if (tabvals) {
		tabs[0].num = tabvals[0];
		tabs[1].num = tabvals[1];
		tabs[2].num = tabvals[2];
	} else {
		tabs[0].num = round(count_ranked_players("", ""));
		tabs[1].num = round(count_clans());
		tabs[2].num = round(count_vanilla_servers());
	}

	html("<nav class=\"section_tabs\">");

	for (i = 0; i < SECTION_TABS_COUNT; i++) {
		if (i == tab)
			html("<a class=\"enabled\">");
		else
			html("<a href=\"%s%s%s\">",
			     tabs[i].url, squery ? "?q=" : "", squery ? url_encode(squery) : "");

		html("%s", tabs[i].title);

		if (tabs[i].num)
			html("<small>%u</small>", tabs[i].num);

		html("</a>");
	}

	html("</nav>");
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

void print_page_nav(const char *url, unsigned pnum, unsigned npages)
{
	/* Number of pages shown before and after the current page */
	static const unsigned extra = 3;
	unsigned i;

	assert(url != NULL);

	html("<nav class=\"pages\">");

	/* Previous button */
	if (pnum == 1)
		html("<a class=\"previous\">Previous</a>");
	else
		html("<a class=\"previous\" href=\"%s?p=%u\">Previous</a>", url, pnum - 1);

	/* Link to first page */
	if (pnum > extra + 1)
		html("<a href=\"%s?p=1\">1</a>", url);
	if (pnum > extra + 2)
		html("<span>...</span>");

	/* Extra pages before */
	for (i = min(extra, pnum - 1); i > 0; i--)
		html("<a href=\"%s?p=%u\">%u</a>", url, pnum - i, pnum - i);

	html("<a class=\"current\">%u</a>", pnum);

	/* Extra pages after */
	for (i = 1; i <= min(extra, npages - pnum); i++)
		html("<a href=\"%s?p=%u\">%u</a>", url, pnum + i, pnum + i);

	/* Link to last page */
	if (pnum + extra + 1 < npages)
		html("<span>...</span>");
	if (pnum + extra < npages)
		html("<a href=\"%s?p=%u\">%u</a>", url, npages, npages);

	/* Next button */
	if (pnum == npages)
		html("<a class=\"next\">Next</a>");
	else
		html("<a class=\"next\" href=\"%s?p=%u\">Next</a>", url, pnum + 1);

	html("</nav>");
}
