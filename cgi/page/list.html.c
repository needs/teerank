#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "database.h"
#include "player.h"
#include "clan.h"
#include "server.h"

/* Global context shared by every lists */
static char *gametype;
static char *map;
static char *order;
static unsigned offset;
static unsigned pnum;

enum pcs {
	PCS_PLAYER,
	PCS_CLAN,
	PCS_SERVER
};

static int print_player_list(void)
{
	struct player player;
	struct sqlite3_stmt *res;
	unsigned nrow;
	const char *sortby;

	char query[512], *queryfmt =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE gametype = ? AND map LIKE ?"
		" ORDER BY %s"
		" LIMIT 100 OFFSET %u";

	if (strcmp(order, "rank") == 0) {
		html_start_player_list(1, 0, pnum);
		sortby = SORT_BY_RANK;
	} else if (strcmp(order, "lastseen") == 0) {
		html_start_player_list(0, 1, pnum);
		sortby = SORT_BY_LASTSEEN;
	} else {
		return EXIT_FAILURE;
	}

	snprintf(query, sizeof(query), queryfmt, sortby, offset);
	foreach_player(query, &player, "ss", gametype, map)
		html_player_list_entry(&player, NULL, 0);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && offset > 0)
		return EXIT_NOT_FOUND;

	html_end_player_list();
	return EXIT_SUCCESS;
}

static int print_clan_list(void)
{
	struct clan clan;
	struct sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_CLAN_COLUMNS
		" FROM players"
		" WHERE" IS_VALID_CLAN
		" GROUP BY clan"
		" ORDER BY nmembers DESC, clan"
		" LIMIT 100 OFFSET ?";

	html_start_clan_list();

	foreach_clan(query, &clan, "u", offset)
		html_clan_list_entry(++offset, clan.name, clan.nmembers);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	html_end_clan_list();
	return EXIT_SUCCESS;
}

static int print_server_list(void)
{
	struct server server;
	struct sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE" IS_VANILLA_CTF_SERVER
		" ORDER BY num_clients DESC"
		" LIMIT 100 OFFSET ?";

	html_start_server_list();

	offset = (pnum - 1) * 100;
	foreach_extended_server(query, &server, "u", offset)
		html_server_list_entry(++offset, &server);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	html_end_server_list();
	return EXIT_SUCCESS;
}

int main_html_list(int argc, char **argv)
{
	enum pcs pcs;
	int ret = EXIT_FAILURE;

	if (argc != 6) {
		fprintf(
			stderr, "usage: %s players|clans|servers"
			" <gametype> <map> <sort_order> <page_number>\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "players") == 0)
		pcs = PCS_PLAYER;
	else if (strcmp(argv[1], "clans") == 0)
		pcs = PCS_CLAN;
	else if (strcmp(argv[1], "servers") == 0)
		pcs = PCS_SERVER;
	else {
		fprintf(stderr, "%s: Should be either \"players\", \"clans\" or \"servers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	gametype = argv[2];
	map = argv[3];
	order = argv[4];

	if (!parse_pnum(argv[5], &pnum))
		return EXIT_FAILURE;

	offset = (pnum - 1) * 100;

	if (strcmp(gametype, "CTF") == 0)
		html_header(&CTF_TAB, "CTF", "/players", NULL);
	else if (strcmp(gametype, "DM") == 0)
		html_header(&DM_TAB, "DM", "/players", NULL);
	else if (strcmp(gametype, "TDM") == 0)
		html_header(&TDM_TAB, "TDM", "/players", NULL);
	else
		html_header(gametype, gametype, "/players", NULL);

	switch (pcs) {
	case PCS_PLAYER:
		print_section_tabs(PLAYERS_TAB, NULL, NULL);
		ret = print_player_list();
		print_page_nav("players", pnum, count_ranked_players("", "") / 100 + 1);
		break;
	case PCS_CLAN:
		print_section_tabs(CLANS_TAB, NULL, NULL);
		ret = print_clan_list();
		print_page_nav("clans", pnum, count_clans() / 100 + 1);
		break;
	case PCS_SERVER:
		print_section_tabs(SERVERS_TAB, NULL, NULL);
		ret = print_server_list();
		print_page_nav("servers", pnum, count_vanilla_servers() / 100 + 1);
		break;
	}

	html_footer("player-list", relurl("/players/%s.json?p=%u", argv[2], pnum));

	return ret;
}
