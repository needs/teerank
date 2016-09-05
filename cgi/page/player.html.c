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
	enum read_player_ret ret;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	hexname = argv[1];
	if (!is_valid_hexname(hexname)) {
		fprintf(stderr, "%s: Not a valid player name\n", hexname);
		return EXIT_NOT_FOUND;
	}

	ret = read_player_info(&player, hexname);

	if (ret == PLAYER_ERROR)
		return EXIT_FAILURE;
	if (ret == PLAYER_NOT_FOUND)
		return EXIT_NOT_FOUND;

	hexname_to_name(hexname, name);
	CUSTOM_TAB.name = name;
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, name, NULL);

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

	char jsonurl[PATH_MAX];
	get_url(jsonurl, sizeof(jsonurl), "/players/%s.json.html", player.name);
	html_footer(NULL, jsonurl);

	return EXIT_SUCCESS;
}
