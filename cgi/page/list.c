#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "database.h"
#include "player.h"
#include "clan.h"
#include "server.h"

static bool JSON = false;

/* Global context shared by every lists */
enum pcs pcs;
static char *gametype;
static char *map;
static char *order;
static unsigned pnum;

static void json_start_player_list(void)
{
	json("{%s:[", "players");
}

static void json_player_list_entry(unsigned nrow, struct player *p)
{
	if (nrow)
		json(",");

	json("{");
	json("%s:%s,", "name", p->name);
	json("%s:%s,", "clan", p->clan);
	json("%s:%d,", "elo", p->elo);
	json("%s:%u,", "rank", p->rank);
	json("%s:%d,", "lastseen", p->lastseen);
	json("%s:%s,", "server_ip", p->server_ip);
	json("%s:%s", "server_port", p->server_port);
	json("}");
}

static void json_end_player_list(unsigned nrow)
{
	json("],%s:%u}", "length", nrow);
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

	if (JSON)
		json_start_player_list();
	else {
		url_t url;
		URL(url, "/players", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_start_player_list(url, pnum, order);
	}

	foreach_player(query, &player, "ss", gametype, map) {
		if (JSON)
			json_player_list_entry(nrow, &player);
		else
			html_player_list_entry(&player, NULL, 0);
	}

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	if (JSON)
		json_end_player_list(nrow);
	else
		html_end_player_list();

	return EXIT_SUCCESS;
}

static void json_start_clan_list(void)
{
	json("{%s:[", "clans");
}

static void json_clan_list_entry(unsigned nrow, struct clan *clan)
{
	if (nrow)
		json(",");

	json("{");
	json("%s:%s,", "name", clan->name);
	json("%s:%u", "nmembers", clan->nmembers);
	json("}");
}

static void json_end_clan_list(unsigned nrow)
{
	json("],%s:%u}", "length", nrow);
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

	if (JSON)
		json_start_clan_list();
	else {
		url_t url;
		URL(url, "/clans", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_start_clan_list(url, pnum, NULL);
	}

	offset = (pnum - 1) * 100;
	foreach_clan(query, &clan, "u", offset) {
		if (JSON)
			json_clan_list_entry(nrow, &clan);
		else
			html_clan_list_entry(++offset, clan.name, clan.nmembers);
	}

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	if (JSON)
		json_end_clan_list(nrow);
	else
		html_end_clan_list();

	return EXIT_SUCCESS;
}

static void json_start_server_list(void)
{
	json("{%s:[", "servers");
}

static void json_server_list_entry(unsigned nrow, struct server *server)
{
	if (nrow)
		json(",");

	json("{");
	json("%s:%s,", "ip", server->ip);
	json("%s:%s,", "port", server->port);
	json("%s:%s,", "name", server->name);
	json("%s:%s,", "gametype", server->gametype);
	json("%s:%s,", "map", server->map);
	json("%s:%u,", "maxplayers", server->max_clients);
	json("%s:%u", "nplayers", server->num_clients);
	json("}");
}

static void json_end_server_list(unsigned nrow)
{
	json("],%s:%u}", "length", nrow);
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

	if (JSON)
		json_start_server_list();
	else {
		url_t url;
		URL(url, "/servers", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
		html_start_server_list(url, pnum, NULL);
	}

	offset = (pnum - 1) * 100;
	foreach_extended_server(query, &server, "u", offset) {
		if (JSON)
			json_server_list_entry(nrow, &server);
		else
			html_server_list_entry(++offset, &server);
	}

	if (!res)
		return EXIT_FAILURE;
	if (!nrow && pnum > 1)
		return EXIT_NOT_FOUND;

	if (JSON)
		json_end_server_list(nrow);
	else
		html_end_server_list();

	return EXIT_SUCCESS;
}

static void parse_list_args(struct url *url)
{
	char *pcs_, *pnum_;
	unsigned i;

	pnum_ = "1";
	pcs_ = "players";
	gametype = "CTF";
	map = NULL;
	order = "rank";

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "p") == 0 && url->args[i].val)
			pnum_ = url->args[i].val;
		if (strcmp(url->args[i].name, "gametype") == 0 && url->args[i].val)
			gametype = url->args[i].val;
		if (strcmp(url->args[i].name, "map") == 0 && url->args[i].val)
			map = url->args[i].val;
		if (strcmp(url->args[i].name, "sort") == 0 && url->args[i].val)
			order = url->args[i].val;
	}

	if (url->ndirs > 0)
		pcs_ = url->dirs[0];

	if (strcmp(pcs_, "players") == 0)
		pcs = PCS_PLAYER;
	else if (strcmp(pcs_, "clans") == 0)
		pcs = PCS_CLAN;
	else if (strcmp(pcs_, "servers") == 0)
		pcs = PCS_SERVER;
	else
		error(400, "%s: Should be either \"players\", \"clans\" or \"servers\"\n", pcs_);

	pnum = parse_pnum(pnum_);
}

void generate_html_list(struct url *url)
{
	url_t urlfmt;

	JSON = false;

	parse_list_args(url);

	if (strcmp(gametype, "CTF") == 0)
		html_header(&CTF_TAB, "CTF", "/players", NULL);
	else if (strcmp(gametype, "DM") == 0)
		html_header(&DM_TAB, "DM", "/players", NULL);
	else if (strcmp(gametype, "TDM") == 0)
		html_header(&TDM_TAB, "TDM", "/players", NULL);
	else
		html_header(gametype, gametype, "/players", NULL);

	struct section_tab tabs[] = {
		{ "Players" }, { "Clans" }, { "Servers" }, { NULL }
	};

	URL(tabs[0].url, "/players", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
	URL(tabs[1].url, "/clans",   PARAM_GAMETYPE(gametype), PARAM_MAP(map));
	URL(tabs[2].url, "/servers", PARAM_GAMETYPE(gametype), PARAM_MAP(map));
	tabs[0].val = count_ranked_players("", "");
	tabs[1].val = count_clans();
	tabs[2].val = count_vanilla_servers();

	switch (pcs) {
	case PCS_PLAYER:
		tabs[0].active = true;
		print_section_tabs(tabs);
		print_player_list();
		URL(urlfmt, "/players", PARAM_GAMETYPE(gametype), PARAM_MAP(map), PARAM_ORDER(order));
		print_page_nav(urlfmt, pnum, tabs[0].val / 100 + 1);
		URL(urlfmt, "/players.json", PARAM_GAMETYPE(gametype), PARAM_MAP(map), PARAM_ORDER(order), PARAM_PAGENUM(pnum));
		break;

	case PCS_CLAN:
		tabs[1].active = true;
		print_section_tabs(tabs);
		print_clan_list();
		URL(urlfmt, "/clans", PARAM_GAMETYPE(gametype), PARAM_MAP(map), PARAM_ORDER(order));
		print_page_nav(urlfmt, pnum, tabs[1].val / 100 + 1);
		URL(urlfmt, "/clans.json", PARAM_GAMETYPE(gametype), PARAM_MAP(map), PARAM_ORDER(order), PARAM_PAGENUM(pnum));
		break;

	case PCS_SERVER:
		tabs[2].active = true;
		print_section_tabs(tabs);
		print_server_list();
		URL(urlfmt, "/servers", PARAM_GAMETYPE(gametype), PARAM_MAP(map), PARAM_ORDER(order));
		print_page_nav(urlfmt, pnum, tabs[2].val / 100 + 1);
		URL(urlfmt, "/servers.json", PARAM_GAMETYPE(gametype), PARAM_MAP(map), PARAM_ORDER(order), PARAM_PAGENUM(pnum));
		break;
	}

	html_footer("player-list", urlfmt);
}

void generate_json_list(struct url *url)
{
	JSON = true;

	parse_list_args(url);

	switch (pcs) {
	case PCS_PLAYER:
		print_player_list();
		break;
	case PCS_CLAN:
		print_clan_list();
		break;
	case PCS_SERVER:
		print_server_list();
		break;
	}
}
