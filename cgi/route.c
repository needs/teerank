#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#include "teerank.h"
#include "route.h"
#include "cgi.h"

struct arg {
	char *name;
	char *val;
};

#define MAX_DIRS 8

struct url {
	unsigned ndirs;
	char *dirs[MAX_DIRS];

	unsigned nargs;
	struct arg args[MAX_ARGS];
};

struct url parse_url(char *uri, char *query)
{
	struct url url = {0};
	char *dir, *name;
	unsigned i;

	if (!(dir = strtok(uri, "/"))) {
		/*
		 * strtok() never return an empty string.  And that's
		 * what we usually want, because "/players/" will be
		 * handled the same than "/players".
		 *
		 * However, the root route doesn't have a name, hence
		 * the default page doesn't either.  So to allow default
		 * page for root directory, we make a special case and
		 * use an empty string.
		 */
		strcpy(uri, "");
		url.dirs[0] = uri;
		url.ndirs = 1;
	} else {
		do {
			if (url.ndirs == MAX_DIRS)
				error(414, NULL);

			url_decode(dir);
			url.dirs[url.ndirs] = dir;
			url.ndirs++;
		} while ((dir = strtok(NULL, "/")));
	}

	/*
	 * Load arg 'name' and 'val' in two steps to not mix up strtok()
	 * instances.
	 */

	if (query[0] == '\0')
		return url;

	name = strtok(query, "&");
	while (name) {
		if (url.nargs == MAX_ARGS)
			error(414, NULL);

		url.args[url.nargs].name = name;
		url.nargs++;
		name = strtok(NULL, "&");
	}

	for (i = 0; i < url.nargs; i++) {
		strtok(url.args[i].name, "=");
		url.args[i].val = strtok(NULL, "=");

		if (url.args[i].val)
			url_decode(url.args[i].val);
	}

	return url;
}

static char *convert_hexname(char *hexname)
{
	char *hex = hexname;
	char *name = hexname;

	assert(hexname != NULL);

	for (; hex[0] && hex[1]; hex += 2, name++)
		*name = hextodec(hex[0]) * 16 + hextodec(hex[1]);

	*name = '\0';
	return hexname;
}

/*
 * Parse url arguments to get page number, and set page arguments with
 * the found sort order and page number.  Player list, clan list and
 * server list share this pattern.
 */
static void set_list_args(
	struct route *route, struct url *url, char *order)
{
	char *pcs = "players";
	char *p = "1";
	char *map = "";
	char *gametype = "CTF";
	unsigned i;

	assert(order != NULL);

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "p") == 0 && url->args[i].val)
			p = url->args[i].val;
		if (strcmp(url->args[i].name, "sort") == 0 && url->args[i].val)
			order = url->args[i].val;
	}

	if (url->ndirs > 0)
		pcs = url->dirs[0];
	if (url->ndirs > 1)
		gametype = url->dirs[1];
	if (url->ndirs > 2)
		map = url->dirs[2];

	route->args[1] = pcs;
	route->args[2] = gametype;
	route->args[3] = map;
	route->args[4] = order;
	route->args[5] = p;
}

/*
 * Page setup callback should all have the same function prototype.  For
 * clarity and readability we introduce this macro to quickly declare a
 * setup callback.
 */
#define PAGE_SETUP(name) \
static void setup_##name(struct route *route, struct url *url)

PAGE_SETUP(html_list)
{
	set_list_args(route, url, "rank");
}
PAGE_SETUP(json_player_list)
{
	set_list_args(route, url, "rank");
}
PAGE_SETUP(json_clan_list)
{
	set_list_args(route, url, "nmembers");
}
PAGE_SETUP(json_server_list)
{
	set_list_args(route, url, "nplayers");
}

PAGE_SETUP(html_player)
{
	route->args[1] = url->dirs[url->ndirs - 1];
}
PAGE_SETUP(json_player)
{
	route->args[1] = convert_hexname(url->dirs[url->ndirs - 1]);

	if (url->nargs == 0)
		route->args[2] = "full";
	else if (url->nargs == 1 && strcmp(url->args[0].name, "short") == 0)
		route->args[2] = "short";
	else
		error(400, "Optional parameter name should be \"short\"");
}
PAGE_SETUP(html_clan)
{
	route->args[1] = url->dirs[url->ndirs - 1];
}
PAGE_SETUP(json_clan)
{
	route->args[1] = convert_hexname(url->dirs[url->ndirs - 1]);
}
PAGE_SETUP(html_server)
{
	route->args[1] = url->dirs[url->ndirs - 1];
}
PAGE_SETUP(json_server)
{
	route->args[1] = url->dirs[url->ndirs - 1];
}

PAGE_SETUP(svg_graph)
{
	route->args[1] = url->dirs[url->ndirs - 2];
}

PAGE_SETUP(html_search)
{
	char *q = NULL;
	unsigned i;

	for (i = 0; i < url->nargs; i++)
		if (strcmp(url->args[i].name, "q") == 0)
			q = url->args[i].val ? url->args[i].val : "";

	if (!q)
		error(400, "Missing 'q' parameter\n");

	if (url->ndirs == 2)
		route->args[1] = url->dirs[1];
	else
		route->args[1] = "players";
	route->args[2] = q;
}

/* URLs for player list looked like "/pages/<pnum>.html" */
PAGE_SETUP(html_teerank2_player_list)
{
	redirect("/players?p=%s", url->dirs[url->ndirs - 1]);
}
static int main_html_teerank2_player_list(int argc, char **argv)
{
	return EXIT_NOT_FOUND;
}

/* URLs for player graph looked like "/players/<hexname>/elo+rank.svg" */
PAGE_SETUP(svg_teerank3_graph)
{
	redirect("/player/%s/historic.svg", url_encode(convert_hexname(url->dirs[url->ndirs - 2])));
}
static int main_svg_teerank3_graph(int argc, char **argv)
{
	return main_svg_graph(argc, argv);
}

/* Empty setup callbacks */
PAGE_SETUP(html_about_json_api) {}
PAGE_SETUP(html_about) {}
PAGE_SETUP(json_about) {}
PAGE_SETUP(html_status) {}
PAGE_SETUP(txt_robots) {}
PAGE_SETUP(xml_sitemap) {}

/*
 * Build the root tree given data in "routes.def".
 */

#define DIR(name) { name, "", NULL, NULL, NULL, { NULL }, (struct route[]) {
#define END() } },

#define HTML(name, func) \
	{ name, ".html", "text/html", setup_html_##func, main_html_##func, { #func } }, \
	{ name, "", "text/html", setup_html_##func, main_html_##func, { #func } },
#define JSON(name, func) \
	{ name, ".json", "text/json", setup_json_##func, main_json_##func, { #func } },
#define TXT(name, func) \
	{ name, ".txt", "text/plain", setup_txt_##func, main_txt_##func, { #func } },
#define XML(name, func) \
	{ name, ".xml", "text/xml", setup_xml_##func, main_xml_##func, { #func } },
#define SVG(name, func) \
	{ name, ".svg", "image/svg+xml", setup_svg_##func, main_svg_##func, { #func } },

static struct route root = DIR("")
#include "routes.def"
}};

static int route_match(struct route *route, char *name)
{
	char *ext;

	/* First, check for extension */
	if (!(ext = strrchr(name, route->ext[0])))
		return 0;
	if (strcmp(ext, route->ext) != 0)
		return 0;

	/*
	 * Then check for raw filename, since file extension is already
	 * tested, cut it.
	 */
	*ext = '\0';

	if (strcmp(route->name, "*") == 0)
		return 1;
	if (strcmp(route->name, name) == 0)
		return 1;

	/* Somehow route doesn't match, uncut file extension. */
	*ext = route->ext[0];
	return 0;
}

/*
 * Find the first child route matching the given name.
 */
static struct route *find_child_route(
	struct route *route, char *name, int onlyleaf)
{
	struct route *r;

	for (r = route->routes; r && r->name; r++) {
		if (onlyleaf && r->routes)
			continue;
		if (route_match(r, name))
			return r;
	}

	return NULL;
}

/*
 * Find a leaf route (if any) matching the given URL.
 */
static struct route *find_route(struct url *url)
{
	struct route *route = &root;
	unsigned i;

	for (i = 0; i < url->ndirs && route; i++)
		route = find_child_route(route, url->dirs[i], i == url->ndirs - 1);

	return route;
}

struct route *do_route(char *uri, char *query)
{
	struct route *route;
	struct url url;

	url = parse_url(uri, query);
	route = find_route(&url);

	if (!route)
		error(404, NULL);

	route->setup(route, &url);

	return route;
}
