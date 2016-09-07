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

	char *filename;

	unsigned nargs;
	struct arg args[MAX_ARGS];
};

static void url_decode(char *str)
{
	char *tmp = str;

	while (*str) {
		if (*str == '%') {
			unsigned byte;
			sscanf(str + 1, "%2x", &byte);
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

	dir = strtok(uri, "/");
	do {
		if (url.ndirs == MAX_DIRS)
			error(414, NULL);

		url.dirs[url.ndirs] = dir;
		url.ndirs++;
	} while ((dir = strtok(NULL, "/")));

	/*
	 * The last directory parsed is considered as a file
	 */

	url.filename = url.dirs[url.ndirs - 1];
	url.ndirs--;

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
 * Static pages are the simplest ones: they always have the same name
 * and content type and they do not depends on user supplied arguments.
 */
#define PAGE(filename, pagename, ctype) {                               \
	filename, { "teerank-page-" #pagename }, ctype,                 \
	NULL, page_##pagename##_main                                    \
}

#define PAGE_HTML(filename, pagename) PAGE(filename, pagename, "text/html")
#define PAGE_JSON(filename, pagename) PAGE(filename, pagename, "text/json")
#define PAGE_TXT(filename, pagename)  PAGE(filename, pagename, "text/plain")
#define PAGE_XML(filename, pagename)  PAGE(filename, pagename, "text/xml")
#define PAGE_SVG(filename, pagename)  PAGE(filename, pagename, "image/svg+xml")

/*
 * Dynamic page are more complex: their content type and main function
 * depends on user supplied arguments.  Thus we attach a function that
 * must initialize the page structure using user supplied arguments.
 */
#define DYNAMIC_PAGE(filename, pagename) {                              \
	filename, { "teerank-page-" #pagename }, NULL,                  \
	init_page_##pagename, NULL                                      \
}

static void init_from_filename(
	struct page *page, struct url *url,
	struct page html_page,
	struct page json_page)
{
	char *arg1, *ext1, *ext2;

	arg1 = strtok(url->filename, ".");
	ext1 = strtok(NULL, ".");
	ext2 = strtok(NULL, ".");

	if (ext1 && strcmp(ext1, "html") == 0 && !ext2)
		*page = html_page;
	else if (ext1 && strcmp(ext1, "json") == 0 && !ext2)
		*page = json_page;
	else
		error(404, NULL);

	page->args[1] = arg1;
}

static void init_page_player_list(struct page *page, struct url *url)
{
	init_from_filename(
		page, url,
		(struct page) PAGE_HTML(NULL, player_list_html),
		(struct page) PAGE_JSON(NULL, player_list_json));
}

static void init_page_clan_list(struct page *page, struct url *url)
{
	init_from_filename(
		page, url,
		(struct page) PAGE_HTML(NULL, clan_list_html),
		(struct page) PAGE_JSON(NULL, clan_list_json));
}

static void init_page_server_list(struct page *page, struct url *url)
{
	init_from_filename(
		page, url,
		(struct page) PAGE_HTML(NULL, server_list_html),
		(struct page) PAGE_JSON(NULL, server_list_json));
}

static void init_page_clan(struct page *page, struct url *url)
{
	init_from_filename(
		page, url,
		(struct page) PAGE_HTML(NULL, clan_html),
		(struct page) PAGE_JSON(NULL, clan_json));
}

static void init_page_player(struct page *page, struct url *url)
{
	init_from_filename(
		page, url,
		(struct page) PAGE_HTML(NULL, player_html),
		(struct page) PAGE_JSON(NULL, player_json));
}

static void init_page_graph(struct page *page, struct url *url)
{
	*page = (struct page) PAGE_SVG(NULL, graph);
	page->args[1] = url->dirs[url->ndirs - 1];
}

static void init_page_search(struct page *page, struct url *url)
{
	if (url->nargs != 1)
		error(400, "Missing 'q' parameter\n");
	if (strcmp(url->args[0].name, "q") != 0)
		error(400, "First and only parameter should be named 'q'\n");
	if (!url->args[0].val)
		url->args[0].val = "";

	*page = (struct page) PAGE_HTML(NULL, search);
	page->args[1] = url->args[0].val;
}

struct directory {
	char *name;

	struct page *pages;
	struct directory *dirs;
};

static struct directory root = {
	"", (struct page[]) {
		PAGE_HTML("about.html", about),
		PAGE_HTML("about-json-api.html", about_json_api),
		PAGE_TXT("robots.txt", robots),
		PAGE_XML("sitemap.xml", sitemap),
		DYNAMIC_PAGE("search", search),
		{ NULL }
	}, (struct directory[]) {
		{
			"clans", (struct page[]) {
				DYNAMIC_PAGE(NULL, clan),
				{ NULL }
			}, (struct directory[]) {
				{
					"pages", (struct page[]) {
						DYNAMIC_PAGE(NULL, clan_list),
						{ NULL }
					}, NULL
				}, { NULL }
			}
		}, {
			"players", (struct page[]) {
				DYNAMIC_PAGE(NULL, player),
				{ NULL }
			}, (struct directory[]) {
				{
					"pages", (struct page[]) {
						DYNAMIC_PAGE(NULL, player_list),
						{ NULL }
					}, NULL
				}, {
					NULL, (struct page[]) {
						DYNAMIC_PAGE("elo+rank.svg", graph),
						{ NULL }
					}, NULL
				}, { NULL }
			}
		}, {
			"servers", NULL,
			(struct directory[]) {
				{
					"pages", (struct page[]) {
						DYNAMIC_PAGE(NULL, server_list),
						{ NULL }
					}, NULL
				}, { NULL }
			}
		}, { NULL }
	}
};

static struct directory *find_directory(const struct directory *parent, char *name)
{
	struct directory *dir;

	assert(parent != NULL);
	assert(name != NULL);

	if (parent->dirs) {
		for (dir = parent->dirs; dir->name; dir++)
			if (!strcmp(name, dir->name))
				return dir;

		/* Fallback on default directory, if any */
		if (dir->pages)
			return dir;
	}

	return NULL;
}

static struct page *find_page(const struct directory *dir, char *name)
{
	struct page *page;

	assert(dir != NULL);
	assert(name != NULL);

	if (!dir->pages)
		return NULL;

	for (page = dir->pages; page->name; page++)
		if (!strcmp(page->name, name))
			return page;

	/* Fallback on default page, if any */
	if (!page->name && page->args[0])
		return page;

	return NULL;
}

struct page *do_route(char *uri, char *query)
{
	const struct directory *dir = &root;
	struct page *page = NULL;
	struct url url;
	unsigned i;

	url = parse_url(uri, query);

	for (i = 0; i < url.ndirs; i++) {
		if (!(dir = find_directory(dir, url.dirs[i])))
			error(404, NULL);
	}

	if (!(page = find_page(dir, url.filename)))
		error(404, NULL);

	if (page->init)
		page->init(page, &url);

	return page;
}
