#include <stdio.h>
#include <stdlib.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"

int main_html_about(struct url *url)
{
	html_header(&ABOUT_TAB, "About", "", NULL);

	html("<h1>About</h1>");
	html("<p>Teerank is an <em>unofficial</em> ranking system for <a href=\"https://www.teeworlds.com/\">Teeworlds</a>.  It aims to be simple and fast to use.  You can find the <a href=\"https://github.com/needs/teerank\">source code on github</a>.  Teerank is a free software under <a href=\"http://www.gnu.org/licenses/gpl-3.0.txt\">GNU GPL 3.0</a>.</p>");
	html("<p>Report any bugs or ideas on <a href=\"https://github.com/needs/teerank/issues\">github issue tracker</a> or send them by e-mail at <a href=\"mailto:needs@mailoo.org\">needs@mailoo.org</a>.</p>");

	html("<h1>JSON API</h1>");
	html("<p>Here is a <a href=\"/about-json-api\">detailed presentation of teerank JSON API</a>.  You are free to use it for any purpose.</p>");

	html("<h1>Under the hood</h1>");
	html("<p>Teerank works by polling CTF servers every 5 minutes.  Based on the score difference between two polls, it compute your score using a custom <a href=\"https://en.wikipedia.org/wiki/Elo_rating_system\">ELO rating system</a>.</p>");
	html("<p>The system as it is cannot know the team you are playing on, it also do not handle any sort of player authentification, thus faking is easy.  That's why Teerank should not be taken too seriously, because it is built on a very naïve aproach.</p>");

	html_footer("about", "/about.json");

	return EXIT_SUCCESS;
}
