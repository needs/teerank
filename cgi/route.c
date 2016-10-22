#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#include "config.h"
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

static unsigned char hextodec(char c)
{
	switch (c) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A':
	case 'a': return 10;
	case 'B':
	case 'b': return 11;
	case 'C':
	case 'c': return 12;
	case 'D':
	case 'd': return 13;
	case 'E':
	case 'e': return 14;
	case 'F':
	case 'f': return 15;
	default: return 0;
	}
}

static void url_decode(char *str)
{
	char *tmp = str;

	while (*str) {
		if (*str == '%') {
			unsigned byte;

			if (str[1] == '\0' || str[2] == '\0')
				break;

			byte = hextodec(str[1]) * 16 + hextodec(str[2]);
			*tmp = *(char*)&byte;
			str += 3;
		} else {
			if (*str == '+')
				*tmp = ' ';
			else
				*tmp = *str;
			str++;
		}
		tmp++;
	}
	*tmp = '\0';
}

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

/*
 * Parse url arguments to get page number, and set page arguments with
 * the found sort order and page number.  Player list, clan list and
 * server list share this pattern.
 */
static void set_pagelist_args(
	struct route *this, struct url *url, char *order)
{
	char *p = "1";
	unsigned i;

	for (i = 0; i < url->nargs; i++)
		if (strcmp(url->args[i].name, "p") == 0 && url->args[i].val)
			p = url->args[i].val;

	this->args[1] = p;

	if (url->ndirs == 2)
		this->args[2] = url->dirs[1];
	else
		this->args[2] = order;
}

static void setup_html_player_list(struct route *this, struct url *url)
{
	set_pagelist_args(this, url, "by-rank");
}
static void setup_json_player_list(struct route *this, struct url *url)
{
	set_pagelist_args(this, url, "by-rank");
}
static void setup_html_clan_list(struct route *this, struct url *url)
{
	set_pagelist_args(this, url, "by-nmembers");
}
static void setup_json_clan_list(struct route *this, struct url *url)
{
	set_pagelist_args(this, url, "by-nmembers");
}
static void setup_html_server_list(struct route *this, struct url *url)
{
	set_pagelist_args(this, url, "by-nplayers");
}
static void setup_json_server_list(struct route *this, struct url *url)
{
	set_pagelist_args(this, url, "by-nplayers");
}

static void setup_html_player(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 1];
}
static void setup_json_player(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 1];
}
static void setup_html_clan(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 1];
}
static void setup_json_clan(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 1];
}
static void setup_html_server(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 1];
}
static void setup_json_server(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 1];
}

static void setup_svg_graph(struct route *this, struct url *url)
{
	this->args[1] = url->dirs[url->ndirs - 2];
}

static void setup_html_search(struct route *this, struct url *url)
{
	char *q = NULL;
	unsigned i;

	for (i = 0; i < url->nargs; i++)
		if (strcmp(url->args[i].name, "q") == 0)
			q = url->args[i].val ? url->args[i].val : "";

	if (!q)
		error(400, "Missing 'q' parameter\n");

	if (url->ndirs == 1)
		this->args[1] = "players";
	else
		this->args[1] = url->dirs[0];
	this->args[2] = q;
}

#if ROUTE_V2_URLS
/* URLs for player list looked like "/pages/<pnum>.html" */
static void setup_html_teerank2_player(struct route *this, struct url *url)
{
	redirect("/players?p=%s", url->dirs[url->ndirs - 1]);
}
static int main_html_teerank2_player(int argc, char **argv)
{
	return main_html_player_list(argc, argv);
}
#endif

/* Empty setup handlers */
static void setup_html_about_json_api(struct route *this, struct url *url) {};
static void setup_html_about(struct route *this, struct url *url) {};
static void setup_json_about(struct route *this, struct url *url) {};
static void setup_html_status(struct route *this, struct url *url) {};
static void setup_txt_robots(struct route *this, struct url *url) {};
static void setup_xml_sitemap(struct route *this, struct url *url) {};

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
