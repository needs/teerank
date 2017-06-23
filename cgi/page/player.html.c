#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "database.h"
#include "html.h"

struct player {
	char *name, name_[NAME_STRSIZE];
	char *clan, clan_[CLAN_STRSIZE];
	time_t lastseen;
	char *server_ip, server_ip_[IP_STRSIZE];
	char *server_port, server_port_[PORT_STRSIZE];

	int elo;
	unsigned rank;
};

static void read_player(sqlite3_stmt *res, struct player *p)
{
	p->name = column_text_copy(res, 0, p->name_, sizeof(p->name_));
	p->clan = column_text_copy(res, 1, p->clan_, sizeof(p->clan_));

	p->lastseen = column_time_t(res, 2);
	p->server_ip = column_text_copy(res, 3, p->server_ip_, sizeof(p->server_ip_));
	p->server_port = column_text_copy(res, 4, p->server_port_, sizeof(p->server_port_));

	p->elo = column_int(res, 5);
	p->rank = column_unsigned(res, 6);
}

void generate_html_player(struct url *url)
{
	url_t urlfmt;
	char *pname = NULL;
	struct player player;
	bool found = false;

	sqlite3_stmt *res;
	unsigned i;

	const char *query =
		"SELECT players.name, clan, lastseen, server_ip, server_port, elo, rank"
		" FROM players LEFT OUTER JOIN ranks ON players.name = ranks.name"
		" WHERE players.name IS ?";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			pname = url->args[i].val;
	}

	if (!pname)
		error(400, "Player name required");

	foreach_row(res, query, "s", pname) {
		read_player(res, &player);
		found = true;
	}
	if (!found)
		error(404, NULL);

	html_header(pname, CUSTOM_TAB, .tab_title = "Player", .subtab_title = pname);

	/* Print player logo, name, clan, rank and elo */
	html("<header id=\"player_header\">");
	html("<img src=\"/images/player.png\"/>");
	html("<div>");
	html("<h1 id=\"player_name\">%s</h1>", pname);

	if (player.clan) {
		URL(urlfmt, "/clan", PARAM_NAME(player.clan));
		html("<p id=\"player_clan\"><a href=\"%S\">%s</a></p>", urlfmt, player.clan);
	}

	html("</div>");

	html("<div>");
	html("<p id=\"player_rank\">#%u (%i ELO)</p>", player.rank, player.elo);
	html("<p id=\"player_lastseen\">");
	player_lastseen_link(
		player.lastseen, player.server_ip, player.server_port);
	html("</p>");
	html("</div>");

	html("</header>");
	html("");
	html("<h2>Historic</h2>");
	URL(urlfmt, "/player/historic.svg", PARAM_NAME(pname));
	html("<object data=\"%S\" type=\"image/svg+xml\"></object>", url);

	URL(urlfmt, "/player.json", PARAM_NAME(pname));
	html_footer("player", urlfmt);
}
