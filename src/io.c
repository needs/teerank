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

void print_footer(void)
{
	fputs(
		"			</section>\n"
		"		</main>\n"
		"	</body>\n"
		"</html>\n",
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
