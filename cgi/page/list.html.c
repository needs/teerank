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
static unsigned pnum;

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

	if (strcmp(order, "rank") == 0)
		sortby = SORT_BY_RANK;
	else if (strcmp(order, "lastseen") == 0)
		sortby = SORT_BY_LASTSEEN;
	else
		return EXIT_FAILURE;

	html_start_player_list(relurl("/players/%s/%s", gametype, map), pnum, order);

	snprintf(query, sizeof(query), queryfmt, sortby, (pnum - 1) * 100);
	foreach_player(query, &player, "ss", gametype, map)
		html_player_list_entry(&player, NULL, 0);

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	html_end_player_list();
	return EXIT_SUCCESS;
}

static int print_clan_list(void)
{
	struct clan clan;
	struct sqlite3_stmt *res;
	unsigned offset, nrow;

	const char *query =
		"SELECT" ALL_CLAN_COLUMNS
		" FROM players"
		" WHERE" IS_VALID_CLAN
		" GROUP BY clan"
		" ORDER BY nmembers DESC, clan"
		" LIMIT 100 OFFSET ?";

	html_start_clan_list(relurl("/clans/%s/%s", gametype, map), pnum, NULL);

	offset = (pnum - 1) * 100;
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
	unsigned offset, nrow;

	const char *query =
		"SELECT" ALL_EXTENDED_SERVER_COLUMNS
		" FROM servers"
		" WHERE" IS_VANILLA_CTF_SERVER
		" ORDER BY num_clients DESC"
		" LIMIT 100 OFFSET ?";

	html_start_server_list(relurl("/servers/%s/%s", gametype, map), pnum, NULL);

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

	ret = parse_list_args(argc, argv, &pcs, &gametype, &map, &order, &pnum);
	if (ret != EXIT_SUCCESS)
		return ret;

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
