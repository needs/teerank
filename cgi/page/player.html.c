#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "database.h"
#include "html.h"
#include "player.h"
#include "json.h"

int main_html_player(int argc, char **argv)
{
	char *pname;
	struct player player;

	sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ?";

	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	pname = argv[1];

	foreach_player(query, &player, "s", pname);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	CUSTOM_TAB.name = escape(pname);
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, pname, "/players", NULL);

	/* Print player logo, name, clan, rank and elo */
	html("<header id=\"player_header\">");
	html("<img src=\"/images/player.png\"/>");
	html("<div>");
	html("<h1 id=\"player_name\">%s</h1>", escape(pname));

	if (*player.clan)
		html("<p id=\"player_clan\"><a href=\"/clan/%s\">%s</a></p>",
		     url_encode(player.clan), player.clan);

	html("</div>");

	html("<div>");
	html("<p id=\"player_rank\">#%u (%d ELO)</p>", player.rank, player.elo);
	html("<p id=\"player_lastseen\">");
	player_lastseen_link(
		player.lastseen, build_addr(player.server_ip, player.server_port));
	html("</p>");
	html("</div>");

	html("</header>");
	html("");
	html("<h2>Historic</h2>");
	html("<object data=\"/player/%s/historic.svg\" type=\"image/svg+xml\"></object>",
	     url_encode(pname));

	html_footer("player", relurl("/players/%s.json", json_hexstring(pname)));

	return EXIT_SUCCESS;
}
