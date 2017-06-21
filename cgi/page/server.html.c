#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"

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

struct server {
	STRING_FIELD(ip, IP_STRSIZE);
	STRING_FIELD(port, PORT_STRSIZE);

	STRING_FIELD(name, NAME_STRSIZE);
	STRING_FIELD(gametype, GAMETYPE_STRSIZE);
	STRING_FIELD(map, MAP_STRSIZE);

	unsigned num_players;
	unsigned num_specs;

	unsigned num_clients;
	unsigned max_clients;
};

static void read_server(struct sqlite3_stmt *res, void *s_)
{
	struct server *s = s_;

	s->ip = column_string(res, 0, s->ip);
	s->port = column_string(res, 1, s->port);

	s->name = column_string(res, 2, s->name);
	s->gametype = column_string(res, 3, s->gametype);
	s->map = column_string(res, 4, s->map);

	s->max_clients = sqlite3_column_int64(res, 5);

	s->num_players = count_rows(
		"SELECT COUNT(1)"
		" FROM server_clients"
		" WHERE ip IS ? AND port IS ? AND ingame = 1",
		"ss", s->ip, s->port);

	s->num_specs = count_rows(
		"SELECT COUNT(1)"
		" FROM server_clients"
		" WHERE ip IS ? AND port IS ? AND ingame = 0",
		"ss", s->ip, s->port);

	s->num_clients = s->num_players + s->num_specs;
}

/* An IPv4 address starts by either "0." or "00." or "000." */
static bool is_ipv4(const char *ip)
{
	return ip[1] == '.' || ip[2] == '.' || ip[3] == '.';
}

static char *build_addr(const char *ip, const char *port)
{
	static char buf[sizeof("[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:00000")];
	int ret;

	if (is_ipv4(ip))
		ret = snprintf(buf, sizeof(buf), "%s:%s", ip, port);
	else
		ret = snprintf(buf, sizeof(buf), "[%s]:%s", ip, port);

	if (ret >= sizeof(buf))
		return NULL;

	return buf;
}

void generate_html_server(struct url *url)
{
	struct server server;
	unsigned i;
	sqlite3_stmt *res;
	unsigned nrow;
	url_t urlfmt;

	char *addr;
	char *ip = DEFAULT_PARAM_VALUE(PARAM_IP(0));
	char *port = DEFAULT_PARAM_VALUE(PARAM_PORT(0));

	const char *query =
		"SELECT ip, port, name, gametype, map, max_clients"
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

	foreach_row(query, read_server, &server, "ss", ip, port);
	if (!res)
		error(500, NULL);
	if (!nrow)
		error(404, NULL);

	/* Eventually, print them */
	html_header(server.name, server.name, "/servers", NULL);
	html("<header id=\"server_header\">");
	html("<section id=\"serverinfo\">");
	html("<h2>%s</h2>", server.name);

	html("<ul>");
	html("<li>%s</li><li>%s</li>", server.gametype, server.map);
	html("<li>%u / %u clients</li>", server.num_clients, server.max_clients);

	html("<li>");
	html("%u players", server.num_players);
	if (server.num_specs)
		html(" + %u spectators", server.num_specs);
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
