#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "database.h"
#include "html.h"

DEFINE_SIMPLE_LIST_CLASS_FUNC(rank_list_class, "ranklist");

struct player {
	char *name, name_[NAME_STRSIZE];
	char *clan, clan_[CLAN_STRSIZE];
	time_t lastseen;
	char *server_ip, server_ip_[IP_STRSIZE];
	char *server_port, server_port_[PORT_STRSIZE];
};

static void read_player(sqlite3_stmt *res, struct player *p)
{
	p->name = column_text_copy(res, 0, p->name_, sizeof(p->name_));
	p->clan = column_text_copy(res, 1, p->clan_, sizeof(p->clan_));

	p->lastseen = column_time_t(res, 2);
	p->server_ip = column_text_copy(res, 3, p->server_ip_, sizeof(p->server_ip_));
	p->server_port = column_text_copy(res, 4, p->server_port_, sizeof(p->server_port_));
}

void generate_html_player(struct url *url)
{
	url_t urlfmt;
	char *pname = NULL;
	struct player player;
	bool found = false;

	sqlite3_stmt *res;
	unsigned i;

	struct html_list_column cols[] = {
		{ "",          NULL, HTML_COLTYPE_RANK },
		{ "Gametype",  NULL, HTML_COLTYPE_GAMETYPE },
		{ "Elo",       NULL, HTML_COLTYPE_ELO },
		{ NULL }
	};

	const char *qmain =
		"SELECT name, clan, lastseen, server_ip, server_port"
		" FROM players"
		" WHERE players.name IS ?";

	const char *qranks =
		"SELECT rank, gametype, elo"
		" FROM ranks"
		" WHERE name IS ? AND map = ''";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			pname = url->args[i].val;
	}

	if (!pname)
		error(400, "Player name required");

	foreach_row(res, qmain, "s", pname) {
		read_player(res, &player);
		found = true;
	}
	if (!found)
		error(404, NULL);

	html_header(CUSTOM_TAB, .title = "Player", .subtab = pname);

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
	html("<p id=\"player_lastseen\">");
	player_lastseen_link(
		player.lastseen, player.server_ip, player.server_port);
	html("</p>");
	html("</div>");

	html("</header>");
	html("");

	res = foreach_init(qranks, "s", pname);
	html_list(res, cols, NULL, rank_list_class, NULL, 0, 0);

	html("<h2>Historic</h2>");
	URL(urlfmt, "/player/historic.svg", PARAM_NAME(pname));
	html("<object data=\"%S\" type=\"image/svg+xml\"></object>", urlfmt);

	URL(urlfmt, "/player.json", PARAM_NAME(pname));
	html_footer("player", urlfmt);
}
