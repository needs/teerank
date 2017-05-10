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

int main_html_server(int argc, char **argv)
{
	struct server server;
	unsigned i, playing = 0, spectating = 0;
	char *ip, *port;
	const char *addr;
	sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	if (argc != 2) {
		fprintf(stderr, "usage: %s <server_addr>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_addr(argv[1], &ip, &port))
		return EXIT_NOT_FOUND;

	foreach_extended_server(query, &server, "ss", ip, port);
	if (!res)
		return EXIT_FAILURE;
	if (!nrow)
		return EXIT_NOT_FOUND;
	if (!read_server_clients(&server))
		return EXIT_FAILURE;

	for (i = 0; i < server.num_clients; i++)
		server.clients[i].ingame ? playing++ : spectating++;

	/* Eventually, print them */
	html_header(server.name, server.name, "/servers", NULL);
	html("<header id=\"server_header\">");
	html("<section id=\"serverinfo\">");
	html("<h2>%s</h2>", escape(server.name));

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

	html_footer("server", relurl("/servers/%s.json", addr));

	return EXIT_SUCCESS;
}
