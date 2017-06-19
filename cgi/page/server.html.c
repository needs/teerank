#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "server.h"

static char *client_list_class(sqlite3_stmt *res)
{
	if (!res)
		return "playerlist";
	else if (sqlite3_column_int(res, 2) == 0)
		return "spectator";
	else
		return NULL;
}

static void show_client_list(char *ip, char *port, char *gametype, char *map)
{
	sqlite3_stmt *res;

	const char *query =
		"SELECT rank, server_clients.name, ingame, server_clients.clan, "
		"  score, elo"
		" FROM server_clients LEFT OUTER JOIN ranks"
		"  ON server_clients.name = ranks.name"
		"   AND gametype IS ? AND map IS ?"
		" WHERE ip IS ? AND port IS ?"
		" ORDER BY ingame DESC, score DESC, elo DESC";

	struct html_list_column cols[] = {
		{ "",     NULL, HTML_COLTYPE_RANK },
		{ "Name", NULL, HTML_COLTYPE_ONLINE_PLAYER },
		{ "Clan", NULL, HTML_COLTYPE_CLAN },
		{ "Score" },
		{ "Elo", NULL, HTML_COLTYPE_ELO },
		{ NULL }
	};

	res = foreach_init(query, "ssss", gametype, map, ip, port);
	html_list(res, cols, "", client_list_class, NULL, 0, 0);
}

void generate_html_server(struct url *url)
{
	struct server server;
	unsigned i, playing = 0, spectating = 0;
	sqlite3_stmt *res;
	unsigned nrow;
	url_t urlfmt;

	char *addr;
	char *ip = DEFAULT_PARAM_VALUE(PARAM_IP(0));
	char *port = DEFAULT_PARAM_VALUE(PARAM_PORT(0));

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip IS ? AND port IS ?";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "ip") == 0)
			ip = url->args[i].val;
		if (strcmp(url->args[i].name, "port") == 0)
			port = url->args[i].val;
	}

	if (!ip)
		error(400, "Missing 'ip' parameter");
	if (!port)
		error(400, "Missing 'port' parameter");

	foreach_extended_server(query, &server, "ss", ip, port);
	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);
	if (!read_server_clients(&server))
		error(500, NULL);

	for (i = 0; i < server.num_clients; i++)
		server.clients[i].ingame ? playing++ : spectating++;

	/* Eventually, print them */
	html_header(server.name, server.name, "/servers", NULL);
	html("<header id=\"server_header\">");
	html("<section id=\"serverinfo\">");
	html("<h2>%s</h2>", server.name);

	html("<ul>");
	html("<li>%s</li><li>%s</li>", server.gametype, server.map);
	html("<li>%u / %u clients</li>", server.num_clients, server.max_clients);

	html("<li>");
	html("%u players", playing);
	if (spectating)
		html(" + %u spectators", spectating);
	html("</li>");
	html("</ul>");
	html("</section>");

	html("<section id=\"serveraddr\">");
	html("<label for=\"serveraddr_input\">Server address</label>");

	addr = build_addr(server.ip, server.port);
	html("<input type=\"text\" value=\"%s\" size=\"%u\" id=\"serveraddr_input\" readonly/>",
	     addr, strlen(addr));
	html("</section>");
	html("</header>");

	show_client_list(server.ip, server.port, server.gametype, server.map);

	URL(urlfmt, "/server.json", PARAM_IP(ip), PARAM_PORT(port));
	html_footer("server", urlfmt);
}
