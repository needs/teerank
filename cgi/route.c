#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>

#include "teerank.h"
#include "route.h"
#include "html.h"
#include "cgi.h"

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

/* URLs for player list looked like "/pages/<pnum>.html" */
static int main_html_teerank2_player_list(struct url *url)
{
	/* FIXME! Semantic changed and redirect() will not work as
	 * intended */
	redirect(URL("/players?p=%s", url->dirs[url->ndirs - 1]));
	return EXIT_FAILURE;
}

/* URLs for player graph looked like "/players/<hexname>/elo+rank.svg" */
static int main_svg_teerank3_graph(struct url *url)
{
	/* FIXME! Semantic changed and redirect() will not work as
	 * intended */
	redirect(URL("/player/historic.svg?name=%s", convert_hexname(url->dirs[url->ndirs - 2])));
	return EXIT_FAILURE;
}

/*
 * It is a special route name used to match everything.  We can't use
 * NULL for that because it's already used for empty names.
 */
#define _ (char*)1

/*
 * The routes tree refers to pages "main()" callbacks.  So we need to
 * declare them before buliding the tree.
 */
#define DIR(name)
#define END()
#define HTML(name, func)                                                \
	int main_html_##func(struct url *url);
#define JSON(name, func)                                                \
	int main_json_##func(struct url *url);
#define TXT(name, func)                                                 \
	int main_txt_##func(struct url *url);
#define XML(name, func)                                                 \
	int main_xml_##func(struct url *url);
#define SVG(name, func)                                                 \
	int main_svg_##func(struct url *url);

#include "routes.def"

/*
 * Build the root tree given data in "routes.def".
 */

#define DIR(name)                                                       \
	{ name, NULL, NULL, NULL, (struct route[]) {
#define END()                                                           \
	{ 0 } } },

#define HTML(name, func)                                                \
	{ name, "html", "text/html", main_html_##func },                \
	{ name, NULL, "text/html", main_html_##func },
#define JSON(name, func)                                                \
	{ name, "json", "text/json", main_json_##func },
#define TXT(name, func)                                                 \
	{ name, "txt", "text/plain", main_txt_##func },
#define XML(name, func)                                                 \
	{ name, "xml", "text/xml", main_xml_##func },
#define SVG(name, func)                                                 \
	{ name, "svg", "image/svg+xml", main_svg_##func },

static struct route root = DIR(NULL)
#include "routes.def"
}};

static bool samestr(const char *a, const char *b)
{
	if (!a || !b)
		return a == b;
	else
		return strcmp(a, b) == 0;
}

static bool route_match(struct route *route, char *name, char *ext)
{
	if (route->name == _ || samestr(route->name, name))
		return route->main ? samestr(route->ext, ext) : true;
	else
		return false;
}

static bool is_valid_route(struct route *route)
{
	return route && (route->main || route->routes);
}

/*
 * Find the first child route matching the given name.
 */
static struct route *find_child_route(
	struct route *route, char *name, char *ext)
{
	struct route *r;

	for (r = route->routes; is_valid_route(r); r++)
		if (route_match(r, name, ext))
			return r;

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
		route = find_child_route(route, url->dirs[i], url->ext);

	if (route && !route->main)
		route = find_child_route(route, NULL, url->ext);

	if (!route || !route->main)
		return NULL;

	return route;
}

struct route *do_route(struct url *url)
{
	struct route *route;
	route = find_route(url);

	if (!route)
		error(404, NULL);

	return route;
}
