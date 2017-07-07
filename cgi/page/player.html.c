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
};

static void read_player(sqlite3_stmt *res, struct player *p)
{
	p->name = column_text_copy(res, 0, p->name_, sizeof(p->name_));
	p->clan = column_text_copy(res, 1, p->clan_, sizeof(p->clan_));

	p->lastseen = column_time_t(res, 2);
}

void generate_html_player(struct url *url)
{
	url_t urlfmt;
	char *pname;
	struct player player;
	bool found = false;
	sqlite3_stmt *res;
	struct list list;

	struct html_list_column cols[] = {
		{ "", HTML_COLTYPE_RANK },
		{ "Gametype", HTML_COLTYPE_GAMETYPE },
		{ "Elo", HTML_COLTYPE_ELO },
		{ NULL }
	};

	const char *qmain =
		"SELECT name, clan, lastseen"
		" FROM players"
		" WHERE name = ?";

	const char *qranks =
		"SELECT rank, gametype, elo"
		" FROM ranks"
		" WHERE name = ? AND map = ''";

	if (!(pname = URL_EXTRACT(url, PARAM_NAME(0))))
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
	player_lastseen(player.lastseen);
	html("</p>");
	html("</div>");

	html("</header>");
	html("");

	list = init_simple_list(qranks, "s", pname);
	html_list(&list, cols, .class = "ranklist");

	html("<h2>Historic</h2>");
	URL(urlfmt, "/player/historic.svg", PARAM_NAME(pname));
	html("<object data=\"%S\" type=\"image/svg+xml\"></object>", urlfmt);

	URL(urlfmt, "/player.json", PARAM_NAME(pname));
	html_footer("player", urlfmt);
}
