#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"

static char *row_class(sqlite3_stmt *res)
{
	if (column_int(res, 2) == 0)
		return "spectator";
	else
		return NULL;
}

static void show_client_list(char *ip, char *port, char *gametype, char *map)
{
	struct list list;

	const char *query =
		"SELECT rank, server_clients.name, ingame, server_clients.clan, "
		"  score, elo"
		" FROM server_clients LEFT OUTER JOIN ranks"
		"  ON server_clients.name = ranks.name"
		"   AND gametype = ? AND map = ?"
		" WHERE ip = ? AND port = ?"
		" ORDER BY ingame DESC, score DESC, elo DESC";

	struct html_list_column cols[] = {
		{ "", HTML_COLTYPE_RANK },
		{ "Name", HTML_COLTYPE_ONLINE_PLAYER },
		{ "Clan", HTML_COLTYPE_CLAN },
		{ "Score" },
		{ "Elo", HTML_COLTYPE_ELO },
		{ NULL }
	};

	list = init_simple_list(query, "ssss", gametype, map, ip, port);
	html_list(&list, cols, .class = "playerlist", .row_class = row_class);
}

struct server {
	char *ip, ip_[IP_STRSIZE];
	char *port, port_[PORT_STRSIZE];

	char *name, name_[SERVERNAME_STRSIZE];
	char *gametype, gametype_[GAMETYPE_STRSIZE];
	char *map, map_[MAP_STRSIZE];

	unsigned num_players;
	unsigned num_specs;

	unsigned num_clients;
	unsigned max_clients;
};

static void read_server(struct sqlite3_stmt *res, struct server *s)
{
	s->ip = column_text_copy(res, 0, s->ip_, sizeof(s->ip_));
	s->port = column_text_copy(res, 1, s->port_, sizeof(s->port_));

	s->name = column_text_copy(res, 2, s->name_, sizeof(s->name_));
	s->gametype = column_text_copy(res, 3, s->gametype_, sizeof(s->gametype_));
	s->map = column_text_copy(res, 4, s->map_, sizeof(s->map_));

	s->max_clients = column_unsigned(res, 5);

	s->num_players = count_rows(
		"SELECT COUNT(1)"
		" FROM server_clients"
		" WHERE ip = ? AND port = ? AND ingame = 1",
		"ss", s->ip, s->port);

	s->num_specs = count_rows(
		"SELECT COUNT(1)"
		" FROM server_clients"
		" WHERE ip = ? AND port = ? AND ingame = 0",
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
	sqlite3_stmt *res;
	url_t urlfmt;
	bool found = false;

	char *addr;
	char *ip;
	char *port;

	const char *query =
		"SELECT ip, port, name, gametype, map, max_clients"
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	ip = URL_EXTRACT(url, PARAM_IP(0));
	port = URL_EXTRACT(url, PARAM_PORT(0));

	if (!ip)
		error(400, "Missing 'ip' parameter");
	if (!port)
		error(400, "Missing 'port' parameter");

	foreach_row(res, query, "ss", ip, port) {
		read_server(res, &server);
		found = true;
	}
	if (!found)
		error(404, NULL);

	html_header(CUSTOM_TAB, .title = "Server", .subtab = server.name);
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
