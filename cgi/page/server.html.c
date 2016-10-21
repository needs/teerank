#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "player.h"
#include "server.h"

static int cmp_client(const void *p1, const void *p2)
{
	const struct client *a = p1, *b = p2;

	/* Ingame players comes first */
	if (a->ingame != b->ingame)
		return a->ingame ? -1 : 1;

	/* We want player with the higher score first */
	if (b->score > a->score)
		return 1;
	if (b->score < a->score)
		return -1;

	return 0;
}

int main_html_server(int argc, char **argv)
{
	struct server server;
	unsigned i, playing = 0, spectating = 0;
	int ret;
	char *ip, *port;
	const char *addr;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <server_addr>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_addr(argv[1], &ip, &port))
		return EXIT_NOT_FOUND;

	if ((ret = read_server(&server, server_filename(ip, port))) != SUCCESS)
		return ret;

	/* Sort client */
	qsort(server.clients, server.num_clients, sizeof(*server.clients), cmp_client);

	for (i = 0; i < server.num_clients; i++)
		server.clients[i].ingame ? playing++ : spectating++;

	/* Eventually, print them */
	CUSTOM_TAB.name = escape(server.name);
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, server.name, "/servers", NULL);
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

	if (server.num_clients) {
		html_start_online_player_list();

		for (i = 0; i < server.num_clients; i++) {
			struct client client = server.clients[i];
			struct player_info player;

			read_player_info(&player, client.name);
			html_online_player_list_entry(&player, &client);
		}

		html_end_online_player_list();
	}

	html_footer("server", relurl("/server/%s.json", addr));

	return EXIT_SUCCESS;
}
