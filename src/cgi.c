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

#include "config.h"
#include "route.h"

static char *reason_phrase(int code)
{
	switch (code) {
	case 200: return "OK";
	case 400: return "Bad Request";
	case 404: return "Not Found";
	case 414: return "Request-URI Too Long";
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

void error(int code, char *fmt, ...)
{
	va_list ap;

	print_error(code);
	if (fmt) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);

		if (config.debug) {
			va_start(ap, fmt);
			vprintf(fmt, ap);
			va_end(ap);
		}
	} else {
		fprintf(stderr, "%d %s\n", code, reason_phrase(code));
	}

	exit(EXIT_FAILURE);
}

static void dump(int fd, FILE *dst)
{
	FILE *file;
	int c;

	assert(fd != -1);

	if (!(file = fdopen(fd, "r")))
		error(500, "fdopen(): %s\n", strerror(errno));

	/* Just dump fd content */
	printf("Content-Type: text/html\n\n");
	while ((c = fgetc(file)) != EOF)
		fputc(c, dst);
	fclose(file);

	close(fd);
}

static int generate(char **args)
{
	int out[2], err[2];
	pid_t pid;

	assert(args != NULL);
	assert(args[0] != NULL);

	verbose("Generating data with '%s'\n", args[0]);

	if (pipe(out) == -1)
		error(500, "pipe(out): %s\n", strerror(errno));

	if (pipe(err) == -1)
		error(500, "pipe(err): %s\n", strerror(errno));

	pid = fork();
	if (pid == -1) {
		error(500, "fork(): %s\n", strerror(errno));
	} else if (pid == 0) {
		/*
		 * Child process just do redirections and execute the program.
		 */

		dup2(err[1], STDERR_FILENO);
		close(err[0]);

		dup2(out[1], STDOUT_FILENO);
		close(out[0]);

		execvp(args[0], args);
		fprintf(stderr, "execvp(%s): %s\n", args[0], strerror(errno));
		exit(EXIT_FAILURE);
	} else {
		/*
		 * Parent process wait for it's child to terminate and dump the
		 * content of the pipe if child's exit status is different than 0.
		 */

		int c;

		close(err[1]);
		close(out[1]);

		wait(&c);
		if (!WIFEXITED(c) || WEXITSTATUS(c) != EXIT_SUCCESS) {
			/* Report error by dumping err[0] */
			print_error(WEXITSTATUS(c));

			fprintf(stderr, "%s: ", args[0]);
			dump(err[0], stderr);

			exit(EXIT_FAILURE);
		}

		close(err[0]);
		return out[0];
	}

	/* Should never be reached */
	return -1;
}

static char *get_path(void)
{
	static char *tmp, path[PATH_MAX];

	if (!(tmp = getenv("PATH_INFO")) && !(tmp = getenv("DOCUMENT_URI")))
		error(500, "PATH_INFO or DOCUMENT_URI not set\n");

	/* Env vars cannot be modified so we have to copy it */
	if (*stpncpy(path, tmp, PATH_MAX) != '\0')
		error(414, NULL);

	return path;
}

static char *get_query(void)
{
	static char *tmp, query[PATH_MAX] = "";

	if (!(tmp = getenv("QUERY_STRING")))
		return query;

	/* Env vars cannot be modified so we have to copy it */
	if (*stpncpy(query, tmp, PATH_MAX) != '\0')
		error(414, NULL);

	return query;
}

int main(int argc, char **argv)
{
	load_config();

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		fprintf(stderr, "This program expect $PATH_INFO or $DOCUMENT_URI to be set, and optionally $QUERY_STRING.\n");
		error(500, NULL);
	}

	dump(generate(do_route(get_path(), get_query())), stdout);

	return EXIT_SUCCESS;
}
