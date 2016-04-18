#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "html.h"

const struct tab CTF_TAB = { "CTF", "/" };
const struct tab ABOUT_TAB = { "About", "/about.html" };
struct tab CUSTOM_TAB = { NULL, NULL };

static void html(char *str)
{
	assert(str != NULL);

	for (; *str; str++) {
		switch (*str) {
		case '<':
			fputs("&lt;", stdout); break;
		case '>':
			fputs("&gt;", stdout); break;
		case '&':
			fputs("&amp;", stdout); break;
		case '"':
			fputs("&quot;", stdout); break;
		default:
			putchar(*str);
		}
	}
}

void html_header(const struct tab *active, char *title, char *search)
{
	assert(active != NULL);
	assert(title != NULL);

	if (active == &CUSTOM_TAB) {
		assert(active->name != NULL);
		assert(active->href != NULL);
	}

	fputs(
		"<!doctype html>\n"
		"<html>\n"
		"	<head>\n"
		"		<meta charset=\"utf-8\" />\n"
		"		<title>Teerank - ",
		stdout);
	html(title);
	fputs(
		"</title>\n"
		"		<link rel=\"stylesheet\" href=\"/style.css\"/>\n"
		"	</head>\n"
		"	<body>\n"
		"		<header>\n"
		"			<a href=\"/\"><img src=\"/images/logo.png\" alt=\"Logo\" /></a>\n"
		"			<form action=\"/search\">\n"
		"				<input name=\"q\" type=\"text\" placeholder=\"Search\"",
		stdout);
	if (search) {
		fputs(" value=\"", stdout); html(search); fputs("\"", stdout);
	}
	fputs(
		"/>\n"
		"				<input type=\"submit\" value=\"\">\n"
		"			</form>\n"
		"		</header>\n"
		"		<main>\n"
		"			<nav>\n",
		stdout);

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
			printf("\t\t\t\t<a%s>%s</a>\n", class, tab->name);
		else
			printf("\t\t\t\t<a href=\"%s\"%s>%s</a>\n",
			       tab->href, class, tab->name);
	}

	fputs(
		"			</nav>\n"
		"			<section>\n",
		stdout);
}

void html_footer(void)
{
	fputs(
		"			</section>\n"
		"		</main>\n"
		"	</body>\n"
		"</html>\n",
		stdout);
}

/*
 chars can size at most 6 bytes, so out string can size at most:
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

void html_print_player(struct player *player, int show_clan_link)
{
	char name[NAME_LENGTH], clan[NAME_LENGTH];

	assert(player != NULL);

	printf("<tr>");

	/* Rank */
	if (player->rank == INVALID_RANK)
		printf("<td>?</td>");
	else
		printf("<td>%u</td>", player->rank);

	/* Name */
	hexname_to_name(player->name, name);
	printf("<td><a href=\"/players/%s.html\">", player->name); html(name); printf("</a></td>");

	/* Clan */
	hexname_to_name(player->clan, clan);
	printf("<td>");
	if (*clan != '\0') {
		if (show_clan_link)
			printf("<a href=\"/clans/%s.html\">", player->clan);
		html(clan);
		if (show_clan_link)
			printf("</a>");
	}
	printf("</td>");

	/* Elo */
	if (player->elo == INVALID_ELO)
		printf("<td>?</td>");
	else
		printf("<td>%d</td>", player->elo);

	printf("</tr>\n");
}
