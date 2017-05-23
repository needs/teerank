#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "database.h"
#include "html.h"
#include "player.h"

int main_html_player(struct url *url)
{
	char *pname = NULL;
	struct player player;

	sqlite3_stmt *res;
	unsigned i, nrow;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE players.name = ? AND gametype = '' AND map = ''";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			pname = url->args[i].val;
	}

	if (!pname)
		return EXIT_NOT_FOUND;

	foreach_player(query, &player, "s", pname);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;

	html_header(pname, pname, "/players", NULL);

	/* Print player logo, name, clan, rank and elo */
	html("<header id=\"player_header\">");
	html("<img src=\"/images/player.png\"/>");
	html("<div>");
	html("<h1 id=\"player_name\">%s</h1>", pname);

	if (*player.clan)
		html("<p id=\"player_clan\"><a href=\"%S\">%s</a></p>",
		     URL("/clan?name=%s", player.clan), player.clan);

	html("</div>");

	html("<div>");
	html("<p id=\"player_rank\">#%u (%i ELO)</p>", player.rank, player.elo);
	html("<p id=\"player_lastseen\">");
	player_lastseen_link(
		player.lastseen, build_addr(player.server_ip, player.server_port));
	html("</p>");
	html("</div>");

	html("</header>");
	html("");
	html("<h2>Historic</h2>");
	html("<object data=\"%S\" type=\"image/svg+xml\"></object>",
	     URL("/player/historic.svg?name=%s", pname));

	html_footer("player", URL("/player.json?name=%s", pname));

	return EXIT_SUCCESS;
}
