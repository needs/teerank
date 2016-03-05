#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "io.h"

const struct tab CTF_TAB = { "CTF", "/" };
const struct tab ABOUT_TAB = { "About", "/about.html" };
struct tab CUSTOM_TAB = { NULL, NULL };

void print_header(const struct tab *active, char *title, char *search)
{
	assert(active != NULL);
	assert(title != NULL);

	if (active == &CUSTOM_TAB) {
		assert(active->name != NULL);
		assert(active->href != NULL);
	}

	fputs(
		"<!doctype html>"
		"<html>"
		"	<head>"
		"		<meta charset=\"utf-8\" />"
		"		<title>Teerank - ",
		stdout);
	html(title);
	fputs(
		"</title>"
		"		<link rel=\"stylesheet\" href=\"/style.css\"/>"
		"	</head>"
		"	<body>"
		"		<header>"
		"			<a href=\"/\"><img src=\"/images/logo.png\" alt=\"Logo\" /></a>"
		"			<form action=\"/search\">"
		"				<input name=\"q\" type=\"text\" placeholder=\"Search\"",
		stdout);
	if (search) {
		fputs(" value=\"", stdout); html(search); fputs("\"", stdout);
	}
	fputs(
		"/>"
		"				<input type=\"submit\" value=\"\">"
		"			</form>"
		"		</header>"
		"		<main>"
		"			<nav>",
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
			printf("<a%s>%s</a>", class, tab->name);
		else
			printf("<a href=\"%s\"%s>%s</a>",
			       tab->href, class, tab->name);
	}

	fputs(
		"			</nav>"
		"			<section>",
		stdout);
}

void print_footer(void)
{
	fputs(
		"			</section>"
		"		</main>"
		"	</body>"
		"</html>",
		stdout);
}

void hex_to_string(const char *hex, char *str)
{
	assert(hex != NULL);
	assert(str != NULL);
	assert(hex != str);

	for (; hex[0] != '0' || hex[1] != '0'; hex += 2, str++) {
		char tmp[3] = { hex[0], hex[1], '\0' };
		*str = strtol(tmp, NULL, 16);
	}

	*str = '\0';
}

void string_to_hex(const char *str, char *hex)
{
	assert(str != NULL);
	assert(hex != NULL);
	assert(str != hex);

	for (; *str; str++, hex += 2)
		sprintf(hex, "%2x", *(unsigned char*)str);
	strcpy(hex, "00");
}

void html(char *str)
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
