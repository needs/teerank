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
	redirect("/players?p=%s", url->dirs[url->ndirs - 1]);
	return EXIT_FAILURE;
}

/* URLs for player graph looked like "/players/<hexname>/elo+rank.svg" */
static int main_svg_teerank3_graph(struct url *url)
{
	/* FIXME! Semantic changed and redirect() will not work as
	 * intended */
	redirect("/player/%s/historic.svg", url_encode(convert_hexname(url->dirs[url->ndirs - 2])));
	return EXIT_FAILURE;
}

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
	{ name, "", NULL, NULL, (struct route[]) {
#define END()                                                           \
	} },

#define HTML(name, func)                                                \
	{ name, ".html", "text/html", main_html_##func },               \
	{ name, "", "text/html", main_html_##func },
#define JSON(name, func)                                                \
	{ name, ".json", "text/json", main_json_##func },
#define TXT(name, func)                                                 \
	{ name, ".txt", "text/plain", main_txt_##func },
#define XML(name, func)                                                 \
	{ name, ".xml", "text/xml", main_xml_##func },
#define SVG(name, func)                                                 \
	{ name, ".svg", "image/svg+xml", main_svg_##func },

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

struct route *do_route(struct url *url)
{
	struct route *route;
	route = find_route(url);

	if (!route)
		error(404, NULL);

	return route;
}
