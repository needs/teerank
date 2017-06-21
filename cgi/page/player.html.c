#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "database.h"
#include "html.h"

struct player {
	STRING_FIELD(name, NAME_STRSIZE);
	STRING_FIELD(clan, NAME_STRSIZE);
	time_t lastseen;
	STRING_FIELD(server_ip, IP_STRSIZE);
	STRING_FIELD(server_port, PORT_STRSIZE);

	int elo;
	unsigned rank;
};

static void read_player(sqlite3_stmt *res, void *p_)
{
	struct player *p = p_;

	p->name = column_string(res, 0, p->name);
	p->clan = column_string(res, 1, p->clan);

	p->lastseen = sqlite3_column_int64(res, 2);
	p->server_ip = column_string(res, 3, p->server_ip);
	p->server_port = column_string(res, 4, p->server_port);

	p->elo = sqlite3_column_int(res, 5);
	p->rank = sqlite3_column_int64(res, 6);
}

void generate_html_player(struct url *url)
{
	url_t urlfmt;
	char *pname = NULL;
	struct player player;

	sqlite3_stmt *res;
	unsigned i, nrow;

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

	foreach_row(query, read_player, &player, "s", pname);
	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);

	html_header(pname, pname, "/players", NULL);

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
