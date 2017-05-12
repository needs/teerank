#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "json.h"
#include "database.h"
#include "player.h"
#include "clan.h"
#include "server.h"

static bool json = false;

/* Global context shared by every lists */
enum pcs pcs;
static char *gametype;
static char *map;
static char *order;
static unsigned pnum;

static void json_start_player_list(void)
{
	printf("{\"players\":[");
}

static void json_player_list_entry(unsigned nrow, struct player *p)
{
	if (nrow)
		putchar(',');

	putchar('{');
	printf("\"name\":\"%s\",", json_hexstring(p->name));
	printf("\"clan\":\"%s\",", json_hexstring(p->clan));
	printf("\"elo\":%d,", p->elo);
	printf("\"rank\":%u,", p->rank);
	printf("\"lastseen\":\"%s\",", json_date(p->lastseen));
	printf("\"server_ip\":\"%s\",", p->server_ip);
	printf("\"server_port\":\"%s\"", p->server_port);
	putchar('}');
}

static void json_end_player_list(unsigned nrow)
{
	printf("],\"length\":%u}", nrow);
}

static int print_player_list(void)
{
	struct player player;
	struct sqlite3_stmt *res;
	unsigned nrow;
	const char *sortby;

	char query[512], *queryfmt =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM" RANKED_PLAYERS_TABLE
		" WHERE gametype = ? AND map = ?"
		" ORDER BY %s"
		" LIMIT 100 OFFSET %u";

	if (strcmp(order, "rank") == 0)
		sortby = SORT_BY_RANK;
	else if (strcmp(order, "lastseen") == 0)
		sortby = SORT_BY_LASTSEEN;
	else {
		fprintf(stderr, "%s: Should be either \"rank\" or \"lastseen\"\n", order);
		return EXIT_FAILURE;
	}


	snprintf(query, sizeof(query), queryfmt, sortby, (pnum - 1) * 100);

	if (json)
		json_start_player_list();
	else
		html_start_player_list(relurl("/players/%s/%s", gametype, map), pnum, order);

	foreach_player(query, &player, "ss", gametype, map) {
		if (json)
			json_player_list_entry(nrow, &player);
		else
			html_player_list_entry(&player, NULL, 0);
	}

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	if (json)
		json_end_player_list(nrow);
	else
		html_end_player_list();

	return EXIT_SUCCESS;
}

static void json_start_clan_list(void)
{
	printf("{\"clans\":[");
}

static void json_clan_list_entry(unsigned nrow, struct clan *clan)
{
	if (nrow)
		putchar(',');

	putchar('{');
	printf("\"name\":\"%s\",", json_hexstring(clan->name));
	printf("\"nmembers\":\"%u\"", clan->nmembers);
	putchar('}');
}

static void json_end_clan_list(unsigned nrow)
{
	printf("],\"length\":%u}", nrow);
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

	if (json)
		json_start_clan_list();
	else
		html_start_clan_list(relurl("/clans/%s/%s", gametype, map), pnum, NULL);

	offset = (pnum - 1) * 100;
	foreach_clan(query, &clan, "u", offset) {
		if (json)
			json_clan_list_entry(nrow, &clan);
		else
			html_clan_list_entry(++offset, clan.name, clan.nmembers);
	}

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	if (json)
		json_end_clan_list(nrow);
	else
		html_end_clan_list();

	return EXIT_SUCCESS;
}

static void json_start_server_list(void)
{
	printf("{\"servers\":[");
}

static void json_server_list_entry(unsigned nrow, struct server *server)
{
	if (nrow)
		putchar(',');

	putchar('{');
	printf("\"ip\":\"%s\",", server->ip);
	printf("\"port\":\"%s\",", server->port);
	printf("\"name\":\"%s\",", json_escape(server->name));
	printf("\"gametype\":\"%s\",", json_escape(server->gametype));
	printf("\"map\":\"%s\",", json_escape(server->map));

	printf("\"maxplayers\":%u,", server->max_clients);
	printf("\"nplayers\":%u", server->num_clients);
	putchar('}');
}

static void json_end_server_list(unsigned nrow)
{
	printf("],\"length\":%u}", nrow);
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

	if (json)
		json_start_server_list();
	else
		html_start_server_list(relurl("/servers/%s/%s", gametype, map), pnum, NULL);

	offset = (pnum - 1) * 100;
	foreach_extended_server(query, &server, "u", offset) {
		if (json)
			json_server_list_entry(nrow, &server);
		else
			html_server_list_entry(++offset, &server);
	}

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	if (json)
		json_end_server_list(nrow);
	else
		html_end_server_list();

	return EXIT_SUCCESS;
}

static int parse_list_args(struct url *url)
{
	char *pcs_, *pnum_;
	unsigned i;

	pnum_ = "1";
	pcs_ = "players";
	gametype = "CTF";
	map = "";
	order = "rank";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "p") == 0 && url->args[i].val)
			pnum_ = url->args[i].val;
		if (strcmp(url->args[i].name, "sort") == 0 && url->args[i].val)
			order = url->args[i].val;
	}

	if (url->ndirs > 0)
		pcs_ = url->dirs[0];
	if (url->ndirs > 1)
		gametype = url->dirs[1];
	if (url->ndirs > 2)
		map = url->dirs[2];

	if (strcmp(pcs_, "players") == 0)
		pcs = PCS_PLAYER;
	else if (strcmp(pcs_, "clans") == 0)
		pcs = PCS_CLAN;
	else if (strcmp(pcs_, "servers") == 0)
		pcs = PCS_SERVER;
	else {
		fprintf(stderr, "%s: Should be either \"players\", \"clans\" or \"servers\"\n", pcs_);
		return EXIT_FAILURE;
	}

	if (!parse_pnum(pnum_, &pnum))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int main_html_list(struct url *url)
{
	char *urlfmt;
	unsigned tabvals[3];
	int ret = EXIT_FAILURE;

	json = false;

	if ((ret = parse_list_args(url)) != EXIT_SUCCESS)
		return ret;

	if (strcmp(gametype, "CTF") == 0)
		html_header(&CTF_TAB, "CTF", "/players", NULL);
	else if (strcmp(gametype, "DM") == 0)
		html_header(&DM_TAB, "DM", "/players", NULL);
	else if (strcmp(gametype, "TDM") == 0)
		html_header(&TDM_TAB, "TDM", "/players", NULL);
	else
		html_header(gametype, gametype, "/players", NULL);

	urlfmt = relurl("/%%s/%s/%s", gametype, map);
	tabvals[0] = count_ranked_players("", "");
	tabvals[1] = count_clans();
	tabvals[2] = count_vanilla_servers();

	switch (pcs) {
	case PCS_PLAYER:
		print_section_tabs(PLAYERS_TAB, urlfmt, tabvals);
		ret = print_player_list();
		urlfmt = relurl("/players/%s/%s?sort=%s&p=%%u", gametype, map, order);
		print_page_nav(urlfmt, pnum, tabvals[0] / 100 + 1);
		break;
	case PCS_CLAN:
		print_section_tabs(CLANS_TAB, urlfmt, tabvals);
		ret = print_clan_list();
		urlfmt = relurl("/clans/%s/%s?sort=%s&p=%%u", gametype, map, order);
		print_page_nav(urlfmt, pnum, tabvals[1] / 100 + 1);
		break;
	case PCS_SERVER:
		print_section_tabs(SERVERS_TAB, urlfmt, tabvals);
		ret = print_server_list();
		urlfmt = relurl("/servers/%s/%s?sort=%s&p=%%u", gametype, map, order);
		print_page_nav(urlfmt, pnum, tabvals[2] / 100 + 1);
		break;
	}

	html_footer("player-list", relurl("/players/%s.json?p=%u", gametype, pnum));

	return ret;
}

int main_json_list(struct url *url)
{
	int ret;

	json = true;

	if ((ret = parse_list_args(url)) != EXIT_SUCCESS)
		return ret;

	switch (pcs) {
	case PCS_PLAYER:
		return print_player_list();
	case PCS_CLAN:
		return print_clan_list();
	case PCS_SERVER:
		return print_server_list();
	}

	return EXIT_FAILURE;
}
