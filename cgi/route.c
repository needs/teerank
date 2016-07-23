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

struct directory {
	char *name;

	struct page *pages;
	struct directory *dirs;
};

static void init_page_about(struct page *page, struct url *url)
{
}

static void init_page_rank_page(struct page *page, struct url *url)
{
	page->args[1] = strtok(url->filename, ".");
}

static void init_page_clan_list(struct page *page, struct url *url)
{
	page->args[1] = strtok(url->filename, ".");
}

static void init_page_clan(struct page *page, struct url *url)
{
	page->args[1] = strtok(url->filename, ".");
}

static void init_page_player(struct page *page, struct url *url)
{
	page->args[1] = strtok(url->filename, ".");
}

static void init_page_graph(struct page *page, struct url *url)
{
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

	page->args[1] = url->args[0].val;
}

static void init_page_robots(struct page *page, struct url *url)
{
}

static void init_page_sitemap(struct page *page, struct url *url)
{
}

#define PAGE_HTML(filename, pagename) {                                 \
	filename, { "teerank-page-" #pagename }, "text/html",           \
	init_page_##pagename, page_##pagename##_main                    \
}
#define PAGE_TXT(filename, pagename) {                                  \
	filename, { "teerank-page-" #pagename }, "text/plain",          \
	init_page_##pagename, page_##pagename##_main                    \
}
#define PAGE_XML(filename, pagename) {                                  \
	filename, { "teerank-page-" #pagename }, "text/xml",            \
	init_page_##pagename, page_##pagename##_main                    \
}
#define PAGE_SVG(filename, pagename) {                                  \
	filename, { "teerank-page-" #pagename }, "image/svg+xml",       \
	init_page_##pagename, page_##pagename##_main                    \
}

static const struct directory root = {
	"", (struct page[]) {
		PAGE_HTML("about.html", about),
		PAGE_HTML("search", search),
		PAGE_TXT("robots.txt", robots),
		PAGE_XML("sitemap.xml", sitemap),
		{ NULL }
	}, (struct directory[]) {
		{
			"pages", (struct page[]) {
				PAGE_HTML(NULL, rank_page),
				{ NULL }
			}, NULL
		}, {
			"clans", (struct page[]) {
				PAGE_HTML(NULL, clan),
				{ NULL }
			}, (struct directory[]) {
				{
					"pages", (struct page[]) {
						PAGE_HTML(NULL, clan_list),
						{ NULL }
					}, NULL
				}
			}
		}, {
			"players", (struct page[]) {
				PAGE_HTML(NULL, player),
				{ NULL }
			}, (struct directory[]) {
				{
					NULL, (struct page[]) {
						PAGE_SVG("elo+rank.svg", graph),
						{ NULL }
					}, NULL
				}
			}
		}
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
