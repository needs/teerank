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
			ret = stpcpy(ret, "&lt;");
		case '>':
			ret = stpcpy(ret, "&gt;");
		case '&':
			ret = stpcpy(ret, "&amp;");
		case '"':
			ret = stpcpy(ret, "&quot;");
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
	html("<nav>");

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

void html_footer(void)
{
	html("</section>");
	html("</main>");

	html("<footer>");
	html("<p>Teerank %d.%d (%s)</p>", TEERANK_VERSION, TEERANK_SUBVERSION,
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
	html("</tr>");
	html("</thead>");
	html("<tbody>");
}

void html_end_player_list(void)
{
	html("</tbody>");
	html("</table>");
}

void html_print_player(struct player_summary *player, int show_clan_link)
{
	char name[NAME_LENGTH], clan[NAME_LENGTH];

	assert(player != NULL);

	html("<tr>");

	/* Rank */
	if (player->rank == UNRANKED)
		html("<td>?</td>");
	else
		html("<td>%u</td>", player->rank);

	/* Name */
	hexname_to_name(player->name, name);
	html("<td><a href=\"/players/%s.html\">%s</a></td>", player->name, escape(name));

	/* Clan */
	hexname_to_name(player->clan, clan);
	if (!show_clan_link || *clan == '\0')
		html("<td>%s</td>", escape(clan));
	else
		html("<td><a href=\"/clans/%s.html\">%s</a></td>",
		     player->clan, escape(clan));

	/* Elo */
	if (player->elo == INVALID_ELO)
		html("<td>?</td>");
	else
		html("<td>%d</td>", player->elo);

	html("</tr>");
}
