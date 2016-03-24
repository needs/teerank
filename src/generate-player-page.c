#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "config.h"
#include "player.h"

int main(int argc, char **argv)
{
	char name[MAX_NAME_HEX_LENGTH], clan[MAX_CLAN_STR_LENGTH];
	struct player player;
	int player_found;

	load_config();
	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	player_found = read_player(&player, argv[1]);

	hex_to_string(argv[1], name);
	CUSTOM_TAB.name = name;
	CUSTOM_TAB.href = "";
	print_header(&CUSTOM_TAB, name, NULL);

	/* Print player logo, name, clan, rank and elo */
	if (player_found) {
		hex_to_string(player.clan, clan);
		printf("<header id=\"player_header\">\n");
		printf("\t<img src=\"/images/player.png\"/>\n");
		printf("\t<section>\n");
		printf("\t\t<h1>%s</h1>\n", name);
		printf("\t\t<p>%s</p>\n", clan);
		printf("\t</section>\n");
		printf("\t<p>#%u (%d ELO)</p>\n", player.rank, player.elo);
		printf("</header>\n");
	} else {
		printf("<p>Player not found</p>\n");
	}

	print_footer();

	return EXIT_SUCCESS;
}
