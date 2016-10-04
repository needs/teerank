#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "html.h"
#include "info.h"

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
	if (!opening_tag && closing_tag)
		indent--;

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
const struct tab ABOUT_TAB = { "About", "/about.html" };
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

#define plural(n) (n), (n) > 1 ? "s" : ""

/*
 * Compute the number of minutes, hours, days, months and years from the
 * given date to now.  Return 1 if the player is online.
 */
static int elapsed_time_since(struct tm *tm, char *text, char **class)
{
	time_t now = time(NULL), ts;
	time_t elapsed_seconds;
	struct tm elapsed;
	char *dummy;

	if (!class)
		class = &dummy;

	/* Make sure elapsed time is positive */
	ts = mktime(tm);
	if (now < ts)
		elapsed_seconds = ts - now;
	else
		elapsed_seconds = now - ts;

	elapsed = *gmtime(&elapsed_seconds);

	if (elapsed.tm_year - 70) {
		sprintf(text, "%d year%s", plural(elapsed.tm_year - 70));
		*class = "years";
		return 0;
	} else if (elapsed.tm_mon) {
		sprintf(text, "%d month%s", plural(elapsed.tm_mon));
		*class = "months";
		return 0;
	} else if (elapsed.tm_mday - 1) {
		sprintf(text, "%d day%s", plural(elapsed.tm_mday - 1));
		*class = "days";
		return 0;
	} else if (elapsed.tm_hour) {
		sprintf(text, "%d hour%s", plural(elapsed.tm_hour));
		*class = "hours";
		return 0;
	} else if (elapsed.tm_min >= 10) {
		sprintf(text, "%d minute%s", plural(elapsed.tm_min - (elapsed.tm_min % 5)));
		*class = "minutes";
		return 0;
	} else {
		sprintf(text, "Online");
		*class = "online";
		return 1;
	}
}

void html_header(
	const struct tab *active, const char *title,
	const char *sprefix, const char *query)
{
	struct info info;
	char text[64];

	assert(active != NULL);
	assert(title != NULL);

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
	if (read_info(&info) && !elapsed_time_since(&info.last_update, text, NULL))
		html("<mark id=\"alert\">Not updated since %s</mark>", text);

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
		html("<a href=\"/about-json-api.html#%s\">JSON Doc</a>", jsonanchor);
		html("<a class=\"active\">HTML</a>");
		html("</nav>");
	}

	html("</main>");

	html("<footer>");
	html("<p>");
	html("<a href=\"https://github.com/needs/teerank/commit/%s\">Teerank %d.%d</a>",
	     CURRENT_COMMIT, TEERANK_VERSION, TEERANK_SUBVERSION);
	html("<a href=\"https://github.com/needs/teerank/tree/%s\">(%s)</a>",
	     CURRENT_BRANCH, STABLE_VERSION ? "stable" : "unstable");
	html("</p>");
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
		html("<th><a href=\"/players/by-rank?p=%u\">Elo%s</a></th>",
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
	int onlinelist,
	const char *hexname, const char *hexclan,
	int elo, unsigned rank, struct tm lastseen,
	int score, int ingame, const char *addr, int no_clan_link)
{
	char name[NAME_LENGTH], clan[NAME_LENGTH];
	const char *specimg = "<img src=\"/images/spectator.png\" title=\"Spectator\"/>";
	int spectator;

	assert(hexname != NULL);
	assert(hexclan != NULL);

	/* Spectators are less important */
	spectator = onlinelist && !ingame;
	if (spectator)
		html("<tr class=\"spectator\">");
	else
		html("<tr>");

	/* Rank */
	if (rank == UNRANKED)
		html("<td>?</td>");
	else
		html("<td>%u</td>", rank);

	/* Name */
	hexname_to_name(hexname, name);
	html("<td>%s<a href=\"/players/%s.html\">%s</a></td>",
	     spectator ? specimg : "", hexname, escape(name));

	/* Clan */
	hexname_to_name(hexclan, clan);
	if (no_clan_link || *clan == '\0')
		html("<td>%s</td>", escape(clan));
	else
		html("<td><a href=\"/clans/%s.html\">%s</a></td>",
		     hexclan, escape(clan));

	/* Score (online-player-list only) */
	if (onlinelist)
		html("<td>%d</td>", score);

	/* Elo */
	if (elo == INVALID_ELO)
		html("<td>?</td>");
	else
		html("<td>%d</td>", elo);

	/* Last seen (not online-player-list only) */
	if (!onlinelist && mktime(&lastseen) == NEVER_SEEN) {
		html("<td></td>");
	} else if (!onlinelist) {
		char text[64], strls[] = "00/00/1970 00h00", *class;
		int online;

		online = elapsed_time_since(&lastseen, text, &class);
		if (strftime(strls, sizeof(strls), "%d/%m/%Y %Hh%M", &lastseen))
			html("<td class=\"%s\" title=\"%s\">", class, strls);
		else
			html("<td class=\"%s\">", class);

		if (online)
			html("<a href=\"/servers/%s.html\">%s</a>", addr, text);
		else
			html("%s", text);

		html("</td>");
	}

	html("</tr>");
}

void html_player_list_entry(
	const char *hexname, const char *hexclan,
	int elo, unsigned rank, struct tm lastseen,
	const char *addr, int no_clan_link)
{
	player_list_entry(0, hexname, hexclan, elo, rank, lastseen, 0, 0, addr, no_clan_link);
}

void html_online_player_list_entry(struct player_info *player, struct client *client)
{
	assert(player != NULL);
	assert(client != NULL);

	player_list_entry(
		1, player->name, player->clan, player->elo, player->rank,
		*gmtime(&NEVER_SEEN), client->score, client->ingame,
		NULL, 0);
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
	unsigned pos, const char *hexname, unsigned nmembers)
{
	char name[NAME_LENGTH];

	assert(hexname != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	hexname_to_name(hexname, name);
	html("<td><a href=\"/clans/%s.html\">%s</a></td>", hexname, escape(name));

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

void html_server_list_entry(unsigned pos, struct indexed_server *server)
{
	assert(server != NULL);

	html("<tr>");

	html("<td>%u</td>", pos);

	/* Name */
	html("<td><a href=\"/servers/%s.html\">%s</a></td>",
	     build_addr(server->ip, server->port), server->name);

	/* Gametype */
	html("<td>%s</td>", server->gametype);

	/* Map */
	html("<td>%s</td>", server->map);

	/* Players */
	html("<td>%u / %u</td>", server->nplayers, server->maxplayers);

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

	return n - (n % (mod / 100));
}

void print_section_tabs(enum section_tab tab, const char *squery, unsigned *tabvals)
{
	unsigned i;

	struct {
		const char *title;
		const char *url;
		unsigned num;
	} default_tabs[] = {
		{ "Players", "/players/by-rank", 0 },
		{ "Clans", "/clans/by-nmembers", 0 },
		{ "Servers", "/servers/by-nplayers", 0 }
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
		struct info info;

		if (read_info(&info)) {
			tabs[0].num = round(info.nplayers);
			tabs[1].num = round(info.nclans);
			tabs[2].num = round(info.nservers);
		}
	}

	html("<nav class=\"section_tabs\">");

	for (i = 0; i < SECTION_TABS_COUNT; i++) {
		if (i == tab)
			html("<a class=\"enabled\">");
		else
			html("<a href=\"%s%s%s\">",
			     tabs[i].url, squery ? "?q=" : "", squery ? squery : "");

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

void print_page_nav(const char *url, struct index_page *ipage)
{
	/* Number of pages shown before and after the current page */
	static const unsigned extra = 3;
	unsigned i;

	unsigned pnum = ipage->pnum;
	unsigned npages = ipage->npages;

	assert(url != NULL);
	assert(ipage != NULL);

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
