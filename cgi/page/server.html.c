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
		return a->ingame ? 1 : -1;

	/* We want player with the higher score first */
	if (b->score > a->score)
		return 1;
	if (b->score < a->score)
		return -1;

	return 0;
}

int page_server_html_main(int argc, char **argv)
{
	struct server server;
	unsigned i, playing = 0, spectating = 0;
	int ret;
	char *ip, *port;

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
	html_header(&CTF_TAB, server.name, NULL);
	html("<h2>%s</h2>", escape(server.name));

	html("<p>%u / %u clients, %u players, %u spectators</p>",
	     server.num_clients, server.max_clients, playing, spectating);

	if (server.num_clients) {
		html_start_player_list(1, 1, 0);

		for (i = 0; i < server.num_clients; i++) {
			struct client client = server.clients[i];
			struct player_info player;

			read_player_info(&player, client.name);
			html_player_list_entry(
				player.name, player.clan, player.elo, player.rank, player.lastseen, 0);
		}

		html_end_player_list();
	}

	html_footer("server");

	return EXIT_SUCCESS;
}
