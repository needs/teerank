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

#include "io.h"
#include "config.h"

/*
 * This program can be used as a CGI or as a native program.  What change
 * between both mode is in how errors are reported and how it gets its input.
 */
static enum mode { CGI, NATIVE } mode;

static char *reason_phrase(int code)
{
	switch (code) {
	case 200: return "OK";
	case 404: return "Not Found";
	case 500: return "Internal Server Error";
	default:  return "";
	}
}

static FILE *start_error(int code)
{
	if (mode == CGI) {
		printf("Content-type: text/html\n");
		printf("Status: %d %s\n\n", code, reason_phrase(code));
		printf("<h1>%d %s</h1>\n", code, reason_phrase(code));
		printf("<pre>");
		return stdout;
	}
	return stderr;
}

static void end_error(void)
{
	if (mode == CGI)
		printf("</pre>");
	exit(EXIT_FAILURE);
}

static void error(int code, char *fmt, ...)
{
	va_list ap;
	FILE *file;

	file = start_error(code);
	if (fmt) {
		va_start(ap, fmt);
		vfprintf(file, fmt, ap);
		va_end(ap);
	}
	end_error();
}

struct file {
	char *name;
	char **args;
	char *source;
};

struct pattern {
	struct file *(*init_file)(char *name, char *source);
	struct file *(*iterate)(void);
};

struct directory {
	char *name;

	struct file *files;
	struct pattern *patterns;
	struct directory *dirs;
};

/* Macros for convenience */
#define foreach_file(dir, f)                                          \
	if ((dir)->files) for ((f) = (dir)->files; (f)->name; (f)++)
#define foreach_pattern(dir, p)                                       \
	if ((dir)->patterns) for ((p) = (dir)->patterns; (p)->init_file; (p)++)
#define foreach_directory(dir, d)                                     \
	if ((dir)->dirs) for ((d) = (dir)->dirs; (d)->name; (d)++)

static void remove_extension(char *str, char *ext)
{
	char *tmp;

	tmp = strrchr(str, '.');
	if (!tmp || strcmp(tmp+1, ext))
		return;
	tmp = '\0';
}

/*
 * Helper function that set file.name and file.source in the following manner:
 *   - set file.name to name if name is not NULL
 *   - set file.source to source if source is not NULL
 *   - set file.name to source with ".html" appended if name is NULL
 *   - set file.source to name with ".html" removed (if any) if source is NULL
 *
 * In every cases config.cache_root and dirtree are prepended for name.
 * In every cases config.root and dirtree are prepended for source.
 */
static struct file *init_file(char *name, char *source, char *dirtree)
{
	static struct file file;
	static char _name[PATH_MAX], _source[PATH_MAX];

	assert(name || source);

	if (name)
		sprintf(_name, "%s/%s/%s", config.cache_root, dirtree, name);
	else
		sprintf(_name, "%s/%s/%s.html", config.cache_root, dirtree, source);

	if (source) {
		sprintf(_source, "%s/%s/%s", config.root, dirtree, source);
	} else {
		sprintf(_source, "%s/%s/%s", config.root, dirtree, name);
		remove_extension(_source, "html");
	}

	file.name = _name;
	file.source = _source;

	return &file;
}

static struct file *init_file_pages(char *name, char *source)
{
	struct file *file;
	static char *args[] = { "teerank-generate-rank-page", "full-page", NULL };

	assert(name || source);

	if (!(file = init_file(name, source, "pages")))
		return NULL;
	file->args = args;

	return file;
}

static struct file *init_file_clans(char *name, char *source)
{
	struct file *file;
	static char *args[] = { "teerank-generate-clan-page", NULL, NULL };

	assert(name || source);

	if (!(file = init_file(name, source, "clans")))
		return NULL;
	args[1] = source;
	file->args = args;

	return file;
}

static struct file *iterate_pages(void)
{
	static struct dirent *dp;
	static DIR *dir = NULL;
	static char path[PATH_MAX] = "";

	if (!path[0])
		sprintf(path, "%s/pages", config.root);

	if (!dir)
		if (!(dir = opendir(path)))
			error(500, "%s: %s\n", path, strerror(errno));

	while ((dp = readdir(dir)))
		if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
			return init_file_pages(NULL, dp->d_name);

	closedir(dir);
	return NULL;
}

static struct file *iterate_clans(void)
{
	static struct dirent *dp;
	static DIR *dir = NULL;
	static char path[PATH_MAX] = "";

	if (!path[0])
		sprintf(path, "%s/clans", config.root);

	if (!dir)
		if (!(dir = opendir(path)))
			error(500, "%s: %s\n", path, strerror(errno));

	while ((dp = readdir(dir)))
		if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
			return init_file_clans(NULL, dp->d_name);

	closedir(dir);
	return NULL;
}

static const struct directory root = {
	"", (struct file[]) {
		{ "index.html", (char*[]){ "teerank-generate-index", NULL } },
		{ "about.html", (char*[]){ "teerank-generate-about", NULL } },
		{ NULL }
	}, NULL, (struct directory[]) {
		{
			"pages", NULL, (struct pattern[]) {
				{ init_file_pages, iterate_pages },
				{ NULL }
			}, NULL
		}, {
			"clans", NULL, (struct pattern[]) {
				{ init_file_clans, iterate_clans },
				{ NULL }
			}, NULL
		}, { NULL }
	}
};

static struct directory *find_directory(const struct directory *_dir, char *name)
{
	struct directory *dir;

	assert(_dir != NULL);
	assert(name != NULL);

	foreach_directory(_dir, dir)
		if (!strcmp(name, dir->name))
			return dir;

	return NULL;
}

static struct file *find_file(const struct directory *dir, char *name)
{
	struct file *file;
	struct pattern *pattern;

	assert(dir != NULL);
	assert(name != NULL);

	foreach_file(dir, file)
		if (!strcmp(file->name, name))
			return file;

	foreach_pattern(dir, pattern)
		if ((file = pattern->init_file(name, NULL)))
			return file;

	return NULL;
}

static int is_cached(char *path, char *dep)
{
	struct stat statpath, statdep;

	assert(path != NULL);

	if (!dep)
		return 0;
	if (stat(path, &statpath) == -1)
		return 0;
	if (stat(dep, &statdep) == -1)
		return 0;
	if (statdep.st_mtim.tv_sec > statpath.st_mtim.tv_sec)
		return 0;

	return 1;
}

static void generate_file(struct file *file)
{
	int dest, src = -1, err[2];
	char tmpname[PATH_MAX];

	assert(file != NULL);

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

	/*
	 * Open src before fork() because if the file doesn't exist, then we
	 * raise a 404, and if it does but cannot be opened, we raise a 500.
	 */
	if (file->source) {
		if ((src = open(file->source, O_RDONLY)) == -1) {
			if (errno == ENOENT)
				error(404, "%s: Doesn't exist\n", file->source);
			else
				error(500, "%s: %s\n", file->source, strerror(errno));
		}
	}

	/* Do not generate if is already cached */
	if (is_cached(file->name, file->source))
		return;

	/* The destination file is a temporary file */
	sprintf(tmpname, "%s/tmp-teerank-XXXXXX", config.tmp_root);
	if ((dest = mkstemp(tmpname)) == -1)
		error(500, "mkstemp(): %s\n", strerror(errno));

	if (pipe(err) == -1)
		error(500, "pipe(): %s\n", strerror(errno));

	/*
	 * Parent process wait for it's child to terminate and dump the
	 * content of the pipe if child's exit status is different than 0.
	 */
	if (fork() > 0) {
		int c;
		FILE *pipefile, *errfile;

		close(err[1]);
		close(dest);

		if (file->source)
			close(src);

		wait(&c);
		if (WIFEXITED(c) && WEXITSTATUS(c) == EXIT_SUCCESS) {
			if (rename(tmpname, file->name) == -1)
				error(500, "rename(%s, %s): %s\n",
				      tmpname, file->name, strerror(errno));
			return;
		}

		/* Report error */
		errfile = start_error(500);
		fprintf(errfile, "%s: ", file->args[0]);
		pipefile = fdopen(err[0], "r");
		while ((c = fgetc(pipefile)) != EOF)
			fputc(c, errfile);
		fclose(pipefile);
		end_error();
	}

	/* Redirect stderr to the write side of the pipe */
	dup2(err[1], STDERR_FILENO);
	close(err[0]);

	/* Redirect stdin */
	if (file->source) {
		dup2(src, STDIN_FILENO);
		close(src);
	}

	/* Redirect stdout to the temporary file */
	dup2(dest, STDOUT_FILENO);
	close(dest);

	/* Eventually, run the program */
	execvp(file->args[0], file->args);
	fprintf(stderr, "execvp(%s): %s\n", file->args[0], strerror(errno));
	exit(EXIT_FAILURE);
}

static void generate_pattern(const struct pattern *pattern)
{
	struct file *file;

	while ((file = pattern->iterate()))
		generate_file(file);
}

static void generate_directory(const struct directory *_dir)
{
	struct directory *dir;
	struct file *file;
	struct pattern *pattern;

	foreach_directory(_dir, dir)
		generate_directory(dir);

	foreach_file(_dir, file)
		generate_file(file);

	foreach_pattern(_dir, pattern)
		generate_pattern(pattern);
}

static void generate(char *path)
{
	const struct directory *dir, *current = &root;
	struct file *file = NULL;
	char *name;

	if (*path != '/')
		error(500, "Path should begin with '/'\n");

	name = strtok(path, "/");
	while (name) {
		if ((dir = find_directory(current, name)))
			current = dir;
		else if ((file = find_file(current, name)))
			break;
		else
			error(404, NULL);
		name = strtok(NULL, "/");
	}

	/* CGI cannot generate more than one file to avoid easy Ddos */
	if (mode == CGI && !file)
		error(404, NULL);

	if (file)
		generate_file(file);
	else
		generate_directory(current);
}

static void print(const char *name)
{
	FILE *file;
	int c;
	char path[PATH_MAX];

	assert(name != NULL);

	sprintf(path, "%s/%s", config.cache_root, name);
	if (!(file = fopen(path, "r")))
		error(500, "%s: %s\n", path, strerror(errno));
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

int main(int argc, char **argv)
{
	load_config();
	init_cache();

	/* No arguments on the command line means it is used as a CGI */
	if (argc > 1) {
		mode = NATIVE;
		while (*++argv)
			generate(*argv);
	} else {
		char *path;

		mode = CGI;
		if (!(path = getenv("DOCUMENT_URI")))
			error(500, "$DOCUMENT_URI not set\n");

		generate(path);
		print(path);
	}

	return EXIT_SUCCESS;
}
