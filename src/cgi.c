#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>

#include "io.h"
#include "config.h"

static char *reason_phrase(int code)
{
	switch (code) {
	case 200: return "OK";
	case 404: return "Not Found";
	case 500: return "Internal Server Error";
	default:  return "";
	}
}

static void print_error(int code)
{
	printf("Content-type: text/html\n");
	printf("Status: %d %s\n\n", code, reason_phrase(code));
	printf("<h1>%d %s</h1>\n", code, reason_phrase(code));
}

static void error(int code, char *fmt, ...)
{
	va_list ap;

	print_error(code);
	if (fmt) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	} else {
		fprintf(stderr, "%d %s\n", code, reason_phrase(code));
	}

	exit(EXIT_FAILURE);
}

struct file {
	char *name;
	char **args;
	char *source;
	char **deps;

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
		error(404, NULL);
	remove_extension(source, "html");

	return source;
}

static char *get_raw_source_from_name(char *name)
{
	static char source[PATH_MAX];

	if (*stpncpy(source, name, PATH_MAX) != '\0')
		error(404, NULL);
	remove_extension(source, "html");

	return source;
}

static struct file *page_default_file(char *name)
{
	static struct file file;
	static char *args[] = { "teerank-generate-rank-page", "full-page", NULL };
	static char *deps[] = { "players", NULL };

	file.name = name;
	file.source = get_source_from_name(name, "pages");
	file.args = args;
	file.deps = deps;

	return &file;
}

static struct file *clan_default_file(char *name)
{
	static struct file file;
	static char *args[] = { "teerank-generate-clan-page", NULL, NULL };
	static char *deps[] = { "players", NULL };

	file.name = name;
	file.source = get_source_from_name(name, "clans");
	file.args = args;
	file.args[1] = get_raw_source_from_name(name);
	file.deps = deps;
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
	static char *next = NULL;
	static struct arg arg;

	if (!next) {
		arg.name = query;
		next = strtok(query, "&");
	} else {
		arg.name = next;
		next = strtok(NULL, "&");
	}

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
	static char *deps[] = { "players", NULL };
	struct arg *arg;

	while ((arg = parse_next_arg(query))) {
		if (arg->name[0] == 'q' && arg->name[1] == '\0')
			break;
	}

	if (!arg)
		error(500, "Missing 'q' parameter");
	else if (!arg->val)
		error(500, "'q' parameter must have a value");

	/* Don't cache search result */
	file->name = NULL;
	file->source = NULL;
	file->args = args;
	file->args[1] = arg->val;
	file->deps = deps;
}

static const struct directory root = {
	"", (struct file[]) {
		{ "index.html", (char*[]){ "teerank-generate-index", NULL },
		  "ranks", (char*[]){ "players", NULL }, NULL },
		{ "about.html", (char*[]){ "teerank-generate-about", NULL },
		  NULL, NULL, NULL },
		{ "search", (char*[]){ "teerank-search", NULL, NULL },
		  NULL, NULL, search_file_apply_query },
		{ NULL }
	}, NULL, (struct directory[]) {
		{ "pages", NULL, page_default_file, NULL, NULL },
		{ "clans", NULL, clan_default_file, NULL, NULL },
		{ NULL }
	}
};

/* Return true if a is more recent than b */
static int is_more_recent(struct stat *a, struct stat *b)
{
	return a->st_mtim.tv_sec > b->st_mtim.tv_sec;
}

static int is_up_to_date(struct file *file)
{
	char **dep;
	struct stat s, tmp;

	assert(file != NULL);

	/* First, if the file is not in cache, it need to be generated */
	if (stat(file->path, &s) == -1)
		return 0;

	/* Regenerate if the source file is more recent */
	if (file->source) {
		if (stat(file->source, &tmp) == -1)
			return 0;
		if (is_more_recent(&tmp, &s))
			return 0;
	}

	/* Regenerate if at least one dependancy is more recent */
	if (file->deps) {
		for (dep = file->deps; *dep; dep++) {
			static char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s/%s", config.root, *dep);
			if (stat(path, &tmp) == -1)
				continue;
			if (is_more_recent(&tmp, &s))
				return 0;
		}
	}

	return 1;
}

static int generate(struct file *file)
{
	int dest[2], src = -1, err[2];
	char tmpname[PATH_MAX];

	assert(file != NULL);
	assert(file->args != NULL);
	assert(file->args[0] != NULL);

	/*
	 * Files are generated by calling a program that write on stdout the
	 * content of the page.
	 *
	 * The program have to write on stdout, and therefor we need to
	 * redirect stdout.  We redirect it to a temporary file created on
	 * purpose, and then use rename() to move the temporary file to his
	 * proper location in cache.  This is done to avoid any race conditions
	 * when two instances of this CGI generate the same file in cache.
	 *
	 * The program may fail and in this case we return an error 500 and
	 * dump the content of stderr.  For that purpose we need to redirect
	 * stderr to a pipe so that the parent can retrieve errors, if any.
	 *
	 * Eventually, the child may need stdin feeded with some data.
	 */

	if (file->path && is_up_to_date(file)) {
		verbose("'%s' already cached and up-to-date\n", file->path);
		if ((dest[0] = open(file->path, O_RDONLY)) == -1)
			error(500, "open(%s): %s\n", file->path, strerror(errno));
		return dest[0];
	} else if (file->path)
		verbose("Generating '%s' with '%s'\n", file->path, file->args[0]);
	else
		verbose("Live generating with '%s'\n", file->args[0]);

	/*
	 * Open src before fork() because if the file doesn't exist, then we
	 * raise a 404, and if it does but cannot be opened, we raise a 500.
	 */
	if (file->source) {
		static char source[PATH_MAX];

		if (snprintf(source, PATH_MAX, "%s/%s", config.root, file->source) >= PATH_MAX)
			error(404, NULL);

		if ((src = open(source, O_RDONLY)) == -1) {
			if (errno == ENOENT)
				error(404, NULL);
			else
				error(500, "%s: %s\n", source, strerror(errno));
		}
	}

	if (file->path) {
		/*
		 * If the file needs to be cached, redirect stdout to a
		 * temporary file and then rename() it later on to the
		 * real destination in cache.
		 */
		if (snprintf(tmpname, PATH_MAX, "%s/tmp-teerank-XXXXXX", config.tmp_root) >= PATH_MAX)
			error(500, "Pathname for temporary file is too long (>= %d)\n", PATH_MAX);
		if ((dest[0] = dest[1] = mkstemp(tmpname)) == -1)
			error(500, "mkstemp(): %s\n", strerror(errno));
	} else {
		/*
		 * If the file do not need to be cached, just create a pipe
		 * to redirect stdout in.
		 */
		if (pipe(dest) == -1)
			error(500, "pipe(dest): %s\n", strerror(errno));
	}

	if (pipe(err) == -1)
		error(500, "pipe(err): %s\n", strerror(errno));

	/*
	 * Parent process wait for it's child to terminate and dump the
	 * content of the pipe if child's exit status is different than 0.
	 */
	if (fork() > 0) {
		int c;
		FILE *pipefile;

		close(err[1]);

		if (file->source)
			close(src);
		if (!file->path)
			close(dest[1]);

		wait(&c);
		if (WIFEXITED(c) && WEXITSTATUS(c) == EXIT_SUCCESS) {
			if (file->path) {
				if (rename(tmpname, file->path) == -1)
					error(500, "rename(%s, %s): %s\n",
					      tmpname, file->path, strerror(errno));
				if (lseek(dest[0], 0, SEEK_SET) == -1)
					error(500, "lseek(%s): %s\n", tmpname, strerror(errno));
			}
			return dest[0];
		}

		/* Report error */
		print_error(500);
		fprintf(stderr, "%s: ", file->args[0]);
		pipefile = fdopen(err[0], "r");
		while ((c = fgetc(pipefile)) != EOF)
			fputc(c, stderr);
		fclose(pipefile);

		exit(EXIT_FAILURE);
	}

	/* Redirect stderr to the write side of the pipe */
	dup2(err[1], STDERR_FILENO);
	close(err[0]);

	/* Redirect stdin */
	if (file->source) {
		dup2(src, STDIN_FILENO);
		close(src);
	}

	/* Redirect stdout */
	dup2(dest[1], STDOUT_FILENO);
	close(dest[1]);

	if (!file->path)
		close(dest[0]);

	/* Eventually, run the program */
	execvp(file->args[0], file->args);
	fprintf(stderr, "execvp(%s): %s\n", file->args[0], strerror(errno));
	exit(EXIT_FAILURE);
}

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

static void undo_strtok(char *str, char *last)
{
	/* For our purpose, we don't remove the last '\0' */
	for (; str != last - 1; str++)
		if (*str == '\0')
			*str = '/';
}

static struct file *parse_uri(char *uri)
{
	const struct directory *tmp, *dir = &root;
	struct file *file = NULL;
	char *name, *query = NULL;
	static char path[PATH_MAX];

	if (*uri != '/')
		error(500, "URI should begin with '/'\n");

	for (name = strtok(uri + 1, "/"); name; name = strtok(NULL, "/")) {
		/* An URI contain only one query, at the end */
		if (query)
			error(404, NULL);
		else if ((query = strchr(name, '?')))
			*query++ = '\0';

		if ((tmp = find_directory(dir, name)))
			dir = tmp;
		else
			break;
	}

	if (!name)
		error(404, NULL);
	else if  (!(file = find_file(dir, name)))
		error(404, NULL);

	if (file->apply_query && query)
		file->apply_query(file, query);

	if (file->name) {
		/*
		 * Construct the file path, since the file name may have been changed
		 * in apply_query(), we must use the updated name instead of the name
		 * provided in the query.
		 */
		undo_strtok(uri, name);
		*(name - 1) = '\0';
		snprintf(path, PATH_MAX, "%s/%s/%s", config.cache_root, uri, file->name);
		file->path = path;
	} else {
		file->path = NULL;
	}

	return file;
}

static void print(int fd)
{
	FILE *file;
	int c;

	assert(fd != -1);

	if (!(file = fdopen(fd, "r")))
		error(500, "fdopen(): %s\n", strerror(errno));
	printf("Content-Type: text/html\n\n");
	while ((c = fgetc(file)) != EOF)
		putchar(c);
	fclose(file);
}

static void create_directory(char *fmt, ...)
{
	va_list ap;
	char path[PATH_MAX];
	int ret;

	va_start(ap, fmt);
	vsprintf(path, fmt, ap);
	va_end(ap);

	if ((ret = mkdir(path, 0777)) == -1)
		if (errno != EEXIST)
			error(500, "%s: %s\n", path, strerror(errno));
}

static void init_cache(void)
{
	create_directory("%s", config.cache_root);
	create_directory("%s/pages", config.cache_root);
	create_directory("%s/clans", config.cache_root);
}

static char *get_uri(void)
{
	static char *tmp, uri[PATH_MAX];

	if (!(tmp = getenv("REQUEST_URI")))
		error(500, "REQUEST_URI not set\n");

	/* Env vars cannot be modified so we have to copy it */
	if (*stpncpy(uri, tmp, PATH_MAX) != '\0')
		error(404, NULL);

	return uri;
}

int main(int argc, char **argv)
{
	load_config();
	init_cache();

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		fprintf(stderr, "This program expect $DOCUMENT_URI to be set to a valid to-be-generated file.\n");
		error(500, NULL);
	}

	print(generate(parse_uri(get_uri())));

	return EXIT_SUCCESS;
}
