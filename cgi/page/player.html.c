#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "player.h"

int main_html_player(int argc, char **argv)
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
	html("<div>");
	html("<h1 id=\"player_name\">%s</h1>", name);

	if (*clan)
		html("<p id=\"player_clan\"><a href=\"/clans/%s\">%s</a></p>", player.clan, clan);

	html("</div>");
	html("<div>");
	html("<p id=\"player_rank\">#%u (%d ELO)</p>", player.rank, player.elo);

	if (mktime(&player.lastseen) != NEVER_SEEN) {
		char text[64], strls[] = "00/00/1970 00h00", *class;
		int is_online;

		is_online = elapsed_time_since(&player.lastseen, &class, text, sizeof(text));

		html("<p id=\"player_lastseen\" class=\"%s\">", class);
		if (is_online) {
			char *addr = build_addr(player.server_ip, player.server_port);
			html("<a href=\"/servers/%s\">%s</a>", addr, text);
		} else if (strftime(strls, sizeof(strls), "%d/%m/%Y %Hh%M", &player.lastseen)) {
			html("%s ago (%s)", text, strls);
		} else {
			html("%s ago", text);
		}
		html("</p>");
	}

	html("</div>");
	html("</header>");
	html("");
	html("<h2>Historic</h2>");
	html("<object data=\"/players/%s/elo+rank.svg\" type=\"image/svg+xml\"></object>",
	     player.name);

	html_footer("player", relurl("/players/%s.json", player.name));

	return EXIT_SUCCESS;
}
