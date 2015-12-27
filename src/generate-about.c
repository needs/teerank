#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "io.h"

int main(int argc, char **argv)
{
	load_config();

	print_header(ABOUT_TAB);

	puts(
		"<h1>About</h1>"
		"<p>Teerank is an <em>unofficial</em> ranking system for <a href=\"https://www.teeworlds.com/\">Teeworlds</a>.  It aims to be simple and fast to use.  You can find the <a href=\"https://github.com/needs/teerank\">source code on github</a>.  Teerank is a free software under <a href=\"http://www.gnu.org/licenses/gpl-3.0.txt\">GNU GPL 3.0</a>.</p>"
		"<p>Report any bugs or ideas on <a href=\"https://github.com/needs/teerank/issues\">github issue tracker</a> or send them by e-mail at <a href=\"mailto:needs@mailoo.org\">needs@mailoo.org</a>.</p>"

		"<h1>Under the hood</h1>"
		"<p>Teerank works by polling CTF servers every 5 minutes.  Based on the score difference between two polls, it compute your score using a custom <a href=\"https://en.wikipedia.org/wiki/Elo_rating_system\">ELO rating system</a>.</p>"
		"<p>The system as it is cannot know the team you are playing on, it also do not handle any sort of player authentification, thus faking is easy.  That's why Teerank should not be taken too seriously, because it is built on a very naïve aproach.</p>"
	);

	print_footer();

	return EXIT_SUCCESS;
}
