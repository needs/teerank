#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "player.h"
#include "server.h"

static void show_client_list(struct server *server)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	struct player p;
	struct client *client;

	const char *query =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ? AND gametype = ? AND map = ?";

	if (!server->num_clients)
		return;

	html_start_online_player_list();

	for (i = 0; i < server->num_clients; i++) {
		client = &server->clients[i];

		foreach_player(query, &p, "sss", client->name, server->gametype, server->map);

		if (res && nrow)
			html_online_player_list_entry(&p, client);
		else
			html_online_player_list_entry(NULL, client);
	}

	html_end_online_player_list();
}

void generate_html_server(struct url *url)
{
	struct server server;
	unsigned i, playing = 0, spectating = 0;
	char *addr, *ip = NULL, *port = NULL;
	sqlite3_stmt *res;
	unsigned nrow;
	url_t urlfmt;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

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

	show_client_list(&server);

	URL(urlfmt, "/server.json", PARAM_IP(ip), PARAM_PORT(port));
	html_footer("server", urlfmt);
}
