#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#include "route.h"
#include "cgi.h"

struct file {
	char *name;
	char **args;
	char *source;

	void (*apply_query)(struct file *file, char *query);

	/* Automatically filled */
	char *path;
};

struct directory {
	char *name;

	struct file *files;
	struct file *(*default_file)(char *name);
	struct directory *dirs;
};

static void remove_extension(char *str, char *ext)
{
	char *tmp;

	tmp = strrchr(str, '.');
	if (!tmp || strcmp(tmp+1, ext))
		return;
	*tmp = '\0';
}

static char *get_source_from_name(char *name, char *dirtree)
{
	static char source[PATH_MAX];

	if (snprintf(source, PATH_MAX, "%s/%s", dirtree, name) >= PATH_MAX)
		error(414, NULL);
	remove_extension(source, "html");

	return source;
}

static char *get_raw_source_from_name(char *name)
{
	static char source[PATH_MAX];

	if (*stpncpy(source, name, PATH_MAX) != '\0')
		error(414, NULL);
	remove_extension(source, "html");

	return source;
}

static struct file *page_default_file(char *name)
{
	static struct file file;
	static char *args[] = { "teerank-generate-rank-page", "full-page", NULL };

	file.name = name;
	file.source = get_source_from_name(name, "pages");
	file.args = args;

	return &file;
}

static struct file *clan_default_file(char *name)
{
	static struct file file;
	static char *args[] = { "teerank-generate-clan-page", NULL, NULL };

	file.name = name;
	file.source = get_source_from_name(name, "clans");
	file.args = args;
	file.args[1] = get_raw_source_from_name(name);

	return &file;
}

struct arg {
	char *name, *val;
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

static struct arg *parse_next_arg(char *query)
{
	static struct arg arg;

	if (!arg.name)
		arg.name = strtok(query, "&");
	else
		arg.name = strtok(NULL, "&");

	if (!arg.name)
		return NULL;

	if ((arg.val = strchr(arg.name, '='))) {
		*arg.val++ = '\0';
		url_decode(arg.val);
	}

	return &arg;
}

static void search_file_apply_query(struct file *file, char *query)
{
	static char *args[] = { "teerank-search", NULL, NULL };
	struct arg *arg;

	while ((arg = parse_next_arg(query))) {
		if (arg->name[0] == 'q' && arg->name[1] == '\0')
			break;
	}

	if (!arg)
		error(400, "Missing 'q' parameter\n");
	else if (!arg->val)
		error(400, "'q' parameter must have a value\n");

	/* Don't cache search result */
	file->name = NULL;
	file->source = NULL;
	file->args = args;
	file->args[1] = arg->val;
}

static const struct directory root = {
	"", (struct file[]) {
		{ "index.html", (char*[]){ "teerank-generate-index", NULL },
		  "ranks", NULL },
		{ "about.html", (char*[]){ "teerank-generate-about", NULL },
		  NULL, NULL, NULL },
		{ "search", (char*[]){ "teerank-search", NULL },
		  NULL, search_file_apply_query, NULL },
		{ NULL }
	}, NULL, (struct directory[]) {
		{ "pages", NULL, page_default_file, NULL, NULL },
		{ "clans", NULL, clan_default_file, NULL, NULL },
		{ NULL }
	}
};

static struct directory *find_directory(const struct directory *parent, char *name)
{
	struct directory *dir;

	assert(parent != NULL);
	assert(name != NULL);

	if (parent->dirs)
		for (dir = parent->dirs; dir->name; dir++)
			if (!strcmp(name, dir->name))
				return dir;

	return NULL;
}

static struct file *find_file(const struct directory *dir, char *name)
{
	struct file *file;

	assert(dir != NULL);
	assert(name != NULL);

	if (dir->files)
		for (file = dir->files; file->name; file++)
			if (!strcmp(file->name, name))
				return file;

	if (dir->default_file)
		return dir->default_file(name);

	return NULL;
}

static struct route *get_route(char *path, struct file *file)
{
	static struct route route;

	route.source = file->source;
	route.args = file->args;

	if (file->name)
		route.cache = path;
	else
		route.cache = NULL;

	return &route;
}

struct route *do_route(char *path, char *query)
{
	const struct directory *tmp, *dir = &root;
	struct file *file = NULL;
	char *name;

	while (*path == '/')
		path++;

	for (name = strtok(path, "/"); name; name = strtok(NULL, "/")) {
		if (name != path)
			*(name - 1) = '/';

		if ((tmp = find_directory(dir, name)))
			dir = tmp;
		else
			break;
	}

	if (!name)
		error(404, NULL);
	if  (!(file = find_file(dir, name)))
		error(404, NULL);

	if (file->apply_query)
		file->apply_query(file, query);

	return get_route(path, file);
}