#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "player.h"

int page_player_html_main(int argc, char **argv)
{
	char name[HEXNAME_LENGTH], clan[NAME_LENGTH];
	const char *hexname;
	struct player_info player;
	int ret;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	hexname = argv[1];
	if (!is_valid_hexname(hexname)) {
		fprintf(stderr, "%s: Not a valid player name\n", hexname);
		return EXIT_NOT_FOUND;
	}

	if ((ret = read_player_info(&player, hexname)) != SUCCESS)
		return ret;

	hexname_to_name(hexname, name);
	CUSTOM_TAB.name = name;
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, name, "/players", NULL);

	/* Print player logo, name, clan, rank and elo */
	hexname_to_name(player.clan, clan);
	html("<header id=\"player_header\">");
	html("<img src=\"/images/player.png\"/>");
	html("<section>");
	html("<h1>%s</h1>", name);

	if (*clan)
		html("<p><a href=\"/clans/%s.html\">%s</a></p>", player.clan, clan);

	html("</section>");
	html("<p>#%u (%d ELO)</p>", player.rank, player.elo);
	html("</header>");
	html("");
	html("<h2>Historic</h2>");
	html("<object data=\"/players/%s/elo+rank.svg\" type=\"image/svg+xml\"></object>",
	     player.name);

	html_footer("player");

	return EXIT_SUCCESS;
}
