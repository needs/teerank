#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "io.h"

void print_header(enum tab active, char *name)
{
	unsigned i;
	static const char before[] =
		"<!doctype html>"
		"<html>"
		"	<head>"
		"		<meta charset=\"utf-8\" />"
		"		<title>Teerank</title>"
		"		<link rel=\"stylesheet\" href=\"/style.css\"/>"
		"	</head>"
		"	<body>"
		"		<header>"
		"			<a href=\"/\"><img src=\"/images/logo.png\" alt=\"Logo\" /></a>"
		"			<form action=\"/search\">"
		"				<input name=\"q\" type=\"text\" placeholder=\"Search\"/>"
		"				<input type=\"submit\" value=\"\">"
		"			</form>"
		"		</header>"
		"		<main>"
		"			<nav>"
		;
	static const char after[] =
		"			</nav>"
		"			<section>"
		;
	struct tab {
		char *name, *href;
	} tabs[] = {
		{ "CTF", "/" },
		{ name, "" },
		{ "About", "/about.html" },
	};

	puts(before);
	for (i = 0; i < TAB_COUNT; i++) {
		if (i != CUSTOM_TAB || active == CUSTOM_TAB) {
			char *class = "";

			if (i == active && active == CUSTOM_TAB)
				class = " class=\"active custom\"";
			else if (i == active && active != CUSTOM_TAB)
				class = " class=\"active\"";

			if (i == active)
				printf("<a%s>%s</a>", class, tabs[i].name);
			else
				printf("<a href=\"%s\"%s>%s</a>",
				       tabs[i].href, class, tabs[i].name);
		}
	}
	puts(after);
}

void print_footer(void)
{
	static const char str[] =
		"			</section>"
		"		</main>"
		"	</body>"
		"</html>"
		;
	puts(str);
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

	do {
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
	} while (*str++);
}
