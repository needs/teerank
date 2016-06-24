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
#define MAX_ARGS 8

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

	name = strtok(query, "&");
	do {
		if (url.nargs == MAX_ARGS)
			error(414, NULL);

		url.args[url.nargs].name = name;
		url.nargs++;
	} while ((name = strtok(NULL, "&")));

	for (i = 0; i < url.nargs; i++) {
		strtok(url.args[i].name, "=");
		url.args[i].val = strtok(NULL, "=");

		if (url.args[i].val)
			url_decode(url.args[i].val);
	}

	return url;
}

struct file {
	char *name;
	char *args[MAX_ARGS];

	void (*init)(struct file *file, struct url *url);
};

struct directory {
	char *name;

	struct file *files;
	struct directory *dirs;
};

static void init_default_page_file(struct file *file, struct url *url)
{
	file->args[1] = "full-page";
	file->args[2] = strtok(url->filename, ".");
}

static void init_default_clan_file(struct file *file, struct url *url)
{
	file->args[1] = strtok(url->filename, ".");
}

static void init_default_player_file(struct file *file, struct url *url)
{
	file->args[1] = strtok(url->filename, ".");
}

static void init_player_elo_graph(struct file *file, struct url *url)
{
	file->args[1] = url->dirs[url->ndirs - 1];
	file->args[2] = "elo";
}

static void init_search_file(struct file *file, struct url *url)
{
	if (url->nargs != 1)
		error(400, "Missing 'q' parameter\n");
	if (strcmp(url->args[0].name, "q") != 0)
		error(400, "First and only parameter should be named 'q'\n");
	if (!url->args[0].val)
		error(400, "'q' parameter must have a value\n");

	file->args[1] = url->args[0].val;
}

static const struct directory root = {
	"", (struct file[]) {
		{ "about.html", { "teerank-html-about" }, NULL },
		{ "search",     { "teerank-html-search" }, init_search_file },
		{ NULL }
	}, (struct directory[]) {
		{ "pages", (struct file[]) {
				{ NULL, { "teerank-html-rank-page" }, init_default_page_file },
				{ NULL }
			}, NULL },
		{ "clans", (struct file[]) {
				{ NULL, { "teerank-html-clan-page" }, init_default_clan_file },
				{ NULL }
			}, NULL },
		{ "players", (struct file[]) {
				{ NULL, { "teerank-html-player-page" }, init_default_player_file },
				{ NULL }
			}, (struct directory[]) {
				{ NULL, (struct file[]) {
						{ "elo.svg", { "teerank-html-graph" }, init_player_elo_graph },
						{ NULL }
					}, NULL },
				{ NULL }
			}
		},
		{ NULL }
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
		if (dir->files)
			return dir;
	}

	return NULL;
}

static struct file *find_file(const struct directory *dir, char *name)
{
	struct file *file;

	assert(dir != NULL);
	assert(name != NULL);

	if (!dir->files)
		return NULL;

	for (file = dir->files; file->name; file++)
		if (!strcmp(file->name, name))
			return file;

	/* Fallback on default file, if any */
	if (!file->name && file->args[0])
		return file;

	return NULL;
}

char **do_route(char *uri, char *query)
{
	const struct directory *dir = &root;
	struct file *file = NULL;
	struct url url;
	unsigned i;

	url = parse_url(uri, query);

	for (i = 0; i < url.ndirs; i++) {
		if (!(dir = find_directory(dir, url.dirs[i])))
			error(404, NULL);
	}

	if (!(file = find_file(dir, url.filename)))
		error(404, NULL);

	if (file->init)
		file->init(file, &url);

	return file->args;
}
