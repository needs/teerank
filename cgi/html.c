#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

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

void html_header(const struct tab *active, char *title, char *search)
{
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
	html("<a href=\"/\"><img src=\"/images/logo.png\" alt=\"Logo\"/></a>");
	html("<form action=\"/search\">");
	html("<input name=\"q\" type=\"text\" placeholder=\"Search\"%s%s%s/>",
	     search ? " value=\"" : "",
	     search ? escape(search) : "",
	     search ? "\"" : "");

	html("<input type=\"submit\" value=\"\"/>");
	html("</form>");
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

void html_footer(const char *jsonanchor)
{
	html("</section>");

	if (jsonanchor) {
		html("<nav id=\"bottabs\">");
		html("<a href=\"/about-json-api.html#%s\">JSON</a>", jsonanchor);
		html("<a class=\"active\">HTML</a>");
		html("</nav>");
	}

	html("</main>");

	html("<footer>");
	html("<p><a href=\"https://github.com/needs/teerank/tree/%s\">Teerank %d.%d</a> <a href=\"https://github.com/needs/teerank/commit/%s\">(%s)</a></p>",
	     CURRENT_BRANCH, TEERANK_VERSION, TEERANK_SUBVERSION, CURRENT_COMMIT,
	     STABLE_VERSION ? "stable" : "unstable");
	html("</footer>");

	html("</body>");
	html("</html>");
}

/*
 * chars can size at most 6 bytes, so out string can size at most:
 * NAME_LENGTH * 6 + 1 bytes.
 */
const char *name_to_html(const char *name)
{
	static char str[NAME_LENGTH * 6 + 1];
	char *it = str;

	assert(name != NULL);

	for (; *name; name++) {
		switch (*name) {
		case '<':
			it = stpcpy(it, "&lt;"); break;
		case '>':
			it = stpcpy(it, "&gt;"); break;
		case '&':
			it = stpcpy(it, "&amp;"); break;
		case '"':
			it = stpcpy(it, "&quot;"); break;
		default:
			*it++ = *name;
		}
	}

	return str;
}

void html_start_player_list(void)
{
	html("<table>");
	html("<thead>");
	html("<tr>");
	html("<th></th>");
	html("<th>Name</th>");
	html("<th>Clan</th>");
	html("<th>Score</th>");
	html("<th>Last seen</th>");
	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

void html_end_player_list(void)
{
	html("</tbody>");
	html("</table>");
}

/*
 * Compute the number of minutes, hours, days, months and years from the
 * given date to now.
 */
static char *elapsed_time_since(struct tm *tm)
{
	time_t now = time(NULL), ts;
	time_t elapsed_seconds;
	struct tm elapsed;
	static char buf[64];

	ts = mktime(tm);
	if (now < ts)
		elapsed_seconds = ts - now;
	else
		elapsed_seconds = now - ts;

	elapsed = *gmtime(&elapsed_seconds);

	if (elapsed.tm_year - 70)
		sprintf(buf, "%d years", elapsed.tm_year - 70);
	else if (elapsed.tm_mon)
		sprintf(buf, "%d months", elapsed.tm_mon);
	else if (elapsed.tm_mday - 1)
		sprintf(buf, "%d days", elapsed.tm_mday - 1);
	else if (elapsed.tm_hour)
		sprintf(buf, "%d hours", elapsed.tm_hour);
	else if (elapsed.tm_min >= 10)
		sprintf(buf, "%d minutes", elapsed.tm_min - (elapsed.tm_min % 5));
	else
		sprintf(buf, "Online");

	return buf;
}

void html_player_list_entry(
	const char *hexname, const char *hexclan, int elo, unsigned rank, struct tm last_seen,
	int no_clan_link)
{
	char name[NAME_LENGTH], clan[NAME_LENGTH];

	assert(hexname != NULL);
	assert(hexclan != NULL);

	html("<tr>");

	/* Rank */
	if (rank == UNRANKED)
		html("<td>?</td>");
	else
		html("<td>%u</td>", rank);

	/* Name */
	hexname_to_name(hexname, name);
	html("<td><a href=\"/players/%s.html\">%s</a></td>", hexname, escape(name));

	/* Clan */
	hexname_to_name(hexclan, clan);
	if (no_clan_link || *clan == '\0')
		html("<td>%s</td>", escape(clan));
	else
		html("<td><a href=\"/clans/%s.html\">%s</a></td>",
		     hexclan, escape(clan));

	/* Elo */
	if (elo == INVALID_ELO)
		html("<td>?</td>");
	else
		html("<td>%d</td>", elo);

	/* Last seen */
	if (mktime(&last_seen) == NEVER_SEEN) {
		html("<td></td>");
	} else {
		html("<td>%s</td>", elapsed_time_since(&last_seen));
	}

	html("</tr>");
}

void print_section_tabs(enum section_tab tab)
{
	unsigned i;

	struct {
		const char *title;
		const char *url;
		unsigned num;
	} tabs[] = {
		{ "Players", "/players/pages/1.html", 80000 },
		{ "Clans", "/clans/pages/1.html", 12000 },
		{ "Servers", "/servers/pages/1.html", 1000 },
	};

	html("<nav class=\"section_tabs\">");

	for (i = 0; i < sizeof(tabs) / sizeof(*tabs); i++) {
		if (i == tab)
			html("<a class=\"enabled\">%s<small>%u</small></a>",
			     tabs[i].title, tabs[i].num);
		else
			html("<a href=\"%s\">%s<small>%u</small></a>",
			     tabs[i].url, tabs[i].title, tabs[i].num);
	}

	html("</nav>");
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

void print_page_nav(const char *prefix, struct index_page *ipage)
{
	/* Number of pages shown before and after the current page */
	static const unsigned EXTRA_PAGES = 3;
	unsigned i;

	unsigned pnum = ipage->pnum;
	unsigned npages = ipage->npages;

	html("<nav class=\"pages\">");
	if (pnum == 1)
		html("<a class=\"previous\">Previous</a>");
	else
		html("<a class=\"previous\" href=\"%s/%u.html\">Previous</a>",
		     prefix, pnum - 1);

	if (pnum > EXTRA_PAGES + 1)
		html("<a href=\"/%s/1.html\">1</a>", prefix);
	if (pnum > EXTRA_PAGES + 2)
		html("<span>...</span>");

	for (i = min(EXTRA_PAGES, pnum - 1); i > 0; i--)
		html("<a href=\"%s/%u.html\">%u</a>",
		     prefix, pnum - i, pnum - i);

	html("<a class=\"current\">%u</a>", pnum);

	for (i = 1; i <= min(EXTRA_PAGES, npages - pnum); i++)
		html("<a href=\"%s/%u.html\">%u</a>",
		     prefix, pnum + i, pnum + i);

	if (pnum + EXTRA_PAGES + 1 < npages)
		html("<span>...</span>");
	if (pnum + EXTRA_PAGES < npages)
		html("<a href=\"%s/%u.html\">%u</a>",
		     prefix, npages, npages);

	if (pnum == npages)
		html("<a class=\"next\">Next</a>");
	else
		html("<a class=\"next\" href=\"%s/%u.html\">Next</a>",
		     prefix, pnum + 1);
	html("</nav>");
}
