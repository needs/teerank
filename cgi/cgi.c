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
#include <ctype.h>

#include "teerank.h"
#include "route.h"
#include "cgi.h"

struct cgi_config cgi_config = {
	"teerank.com", "80"
};

int parse_pnum(const char *str, unsigned *pnum)
{
	long ret;

	errno = 0;
	ret = strtol(str, NULL, 10);
	if (ret == 0 && errno != 0)
		return perror(str), 0;

	/*
	 * Page numbers are unsigned but strtol() returns a long, so we
	 * need to make sure our page number fit into an unsigned.
	 */
	if (ret < 1) {
		fprintf(stderr, "%s: Must be positive\n", str);
		return 0;
	} else if (ret > UINT_MAX) {
		fprintf(stderr, "%s: Must lower than %u\n", str, UINT_MAX);
		return 0;
	}

	*pnum = ret;

	return 1;
}

static int safe_for_urls(char c)
{
	if (isalnum(c))
		return 1;

	return strchr("-_.~", c) != NULL;
}

static char dectohex(char dec)
{
	switch (dec) {
	case 0: return '0';
	case 1: return '1';
	case 2: return '2';
	case 3: return '3';
	case 4: return '4';
	case 5: return '5';
	case 6: return '6';
	case 7: return '7';
	case 8: return '8';
	case 9: return '9';
	case 10: return 'A';
	case 11: return 'B';
	case 12: return 'C';
	case 13: return 'D';
	case 14: return 'E';
	case 15: return 'F';
	default: return '0';
	}
}

char *url_encode(const char *str)
{
	static char _buf[1024];
	char *buf = _buf;

	for (; *str; str++) {
		if (buf - _buf + 1 >= sizeof(_buf))
			break;

		if (safe_for_urls(*str)) {
			*buf++ = *str;
			continue;
		}

		if (buf - _buf + 3 >= sizeof(_buf))
			break;

		*buf++ = '%';
		*buf++ = dectohex(*(unsigned char*)str / 16);
		*buf++ = dectohex(*(unsigned char*)str % 16);
	}

	*buf = '\0';
	return _buf;
}

unsigned char hextodec(char c)
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

void url_decode(char *str)
{
	char *tmp = str;

	while (*str) {
		if (*str == '%') {
			unsigned char byte;

			if (!str[1] || !str[2])
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

char *relurl(const char *fmt, ...)
{
	static char url[PATH_MAX];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(url, sizeof(url), fmt, ap);
	va_end(ap);

	return url;
}

char *absurl(const char *fmt, ...)
{
	static char url[PATH_MAX];
	int ret;

	ret = snprintf(url, sizeof(url), "http://%s", cgi_config.domain);

	if (ret < sizeof(url)) {
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(url + ret, sizeof(url) - ret, fmt, ap);
		va_end(ap);
	}

	return url;
}

static char *reason_phrase(int code)
{
	switch (code) {
	case 200: return "OK";
	case 301: return "Moved Permanently";
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
	printf("Status: %d %s\n", code, reason_phrase(code));
	printf("\n");
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

		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	} else {
		fprintf(stderr, "%d %s\n", code, reason_phrase(code));
	}

	exit(EXIT_FAILURE);
}

void redirect(const char *fmt, ...)
{
	va_list ap;

	assert(fmt != NULL);
	assert(fmt[0] == '/');

	printf("Status: %d %s\n", 301, reason_phrase(301));
	printf("Location: http://%s", cgi_config.domain);

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	putchar('\n');
	putchar('\n');

	exit(EXIT_FAILURE);
}

/*
 * And http status is passed to the function because in order to dum the
 * content of fd, we need to fdopen() it, and it can fail.  If that fail
 * we want to print an error.
 */
static int dump(int status, const char *content_type, int fd, FILE *copy)
{
	FILE *file;
	int c;

	assert(fd != -1);

	if (!(file = fdopen(fd, "r")))
		error(500, "fdopen(): %s\n", strerror(errno));

	if (status != 200) {
		print_error(status);
	} else {
		printf("Content-Type: %s\n", content_type);
		printf("\n");
	}

	if (copy) {
		while ((c = fgetc(file)) != EOF) {
			putchar(c);
			fputc(c, copy);
		}
	} else {
		while ((c = fgetc(file)) != EOF)
			putchar(c);
	}

	fclose(file);
	close(fd);

	return 1;
}

static void raw_dump(int fd, FILE *dst)
{
	FILE *src;
	int c;

	if (!(src = fdopen(fd, "r")))
		error(500, "fdopen(): %s\n", strerror(errno));
	while ((c = fgetc(src)) != EOF)
		fputc(c, dst);
	fclose(src);
	close(fd);
}

static int route_argc(struct route *route)
{
	int i;

	for (i = 0; i < MAX_ARGS && route->args[i]; i++)
		;

	return i;
}

static int generate(struct route *route)
{
	int out[2], err[2];
	int stdout_save, stderr_save;
	int ret;

	assert(route != NULL);

	verbose("Generating data with '%s'", route->args[0]);

	/*
	 * Create a pipe to redirect stdout and stderr to.  It is
	 * necessary because when a failure happen we don't want to send
	 * any content generated before the failure.  Intsead we want to
	 * print something from the error stream.
	 */
	if (pipe(out) == -1)
		error(500, "pipe(out): %s\n", strerror(errno));
	if (pipe(err) == -1)
		error(500, "pipe(err): %s\n", strerror(errno));

	/*
	 * Duplicate stdout and stderr so we can replace them with our
	 * previsouly created pipes, and then restore them to their
	 * actual value once route generation is done.
	 */

	stdout_save = dup(STDOUT_FILENO);
	if (stdout_save == -1)
		error(500, "dup(out): %s\n", strerror(errno));

	stderr_save = dup(STDERR_FILENO);
	if (stderr_save == -1)
		error(500, "dup(err): %s\n", strerror(errno));

	/* Replace stdout and stderr */
	if (dup2(out[1], STDOUT_FILENO) == -1)
		error(500, "dup2(out): %s\n", strerror(errno));
	if (dup2(err[1], STDERR_FILENO) == -1)
		error(500, "dup2(err): %s\n", strerror(errno));
	close(out[1]);
	close(err[1]);

	/* Run route generation */
	ret = route->main(route_argc(route), route->args);

	/*
	 * Some data may not be written yet to the pipe.  Dumping data
	 * now will just raise an empty string.  We need to flush both
	 * stdout and stderr in order to dump generated content.
	 */
	fflush(stdout);
	fflush(stderr);

	/*
	 * Then, restore stdout and stderr, route output is waiting at
	 * the read-end of the pipe "out[0]", and any error are waiting
	 * at the read-end of the pipe "err[0]".
	 */
	if (dup2(stdout_save, STDOUT_FILENO) == -1)
		error(500, "dup2(out, save): %s\n", strerror(errno));
	if (dup2(stderr_save, STDERR_FILENO) == -1)
		error(500, "dup2(err, save): %s\n", strerror(errno));
	close(stdout_save);
	close(stderr_save);

	if (ret == EXIT_SUCCESS) {
		raw_dump(err[0], stderr);
		return dump(200, route->content_type, out[0], NULL);
	}
	if (ret == EXIT_FAILURE)
		return dump(500, NULL, err[0], stderr);
	if (ret == EXIT_NOT_FOUND)
		return dump(404, NULL, err[0], stderr);

	return dump(500, NULL, err[0], stderr);
}

static int load_path_and_query(char **_path, char **_query)
{
	static char path[1024], query[1024];
	char *uri, *tmp;

	/*
	 * Turns out webservers can do some crazy things before giving
	 * us the requested URI.  For instance Nginx does de-encode %2F
	 * ('/') in URL body, making URLs like "/player/foo%2F" not
	 * working because we will receive "/player/foo/".
	 *
	 * Hopefully nginx does provide the unparsed string in
	 * $REQUEST_URI.  We need to split the URI body (path) from the
	 * query string.
	 */

	if (!(uri = getenv("REQUEST_URI")))
		return 0;

	*_path = path;
	*_query = query;
	path[0] = 0;
	query[0] = 0;

	/* Env vars cannot be modified so copy them */
	snprintf(path, sizeof(path), "%s", strtok(uri, "?"));
	tmp = strtok(NULL, "?");
	snprintf(query, sizeof(query), "%s", tmp ? tmp : "");

	return 1;
}

/* Load extra environment variables set by the webserver */
static void init_cgi(void)
{
	const char *tmp, *port = NULL;
	int ret;

	if ((tmp = getenv("SERVER_NAME")))
		cgi_config.name = tmp;
	if ((tmp = getenv("SERVER_PORT")))
		cgi_config.port = tmp;

	if (!cgi_config.name)
		fprintf(stderr, "Warning, SERVER_NAME not set, defaulting to \"teerank.com\"\n");

	if (strcmp(cgi_config.port, "80") != 0)
		port = cgi_config.port;

	ret = snprintf(
		cgi_config.domain, MAX_DOMAIN_LENGTH, "%s%s%s",
		cgi_config.name, port ? ":" : "", port ? port : "");

	if (ret >= MAX_DOMAIN_LENGTH)
		error(414, "%s: Server name too long", cgi_config.name);
}

int main(int argc, char **argv)
{
	char *path, *query;

	/*
	 * We want to use read only mode to prevent any security exploit
	 * to be able to write the database.
	 */
	init_teerank(READ_ONLY);
	init_cgi();

	if (argc != 1 || !load_path_and_query(&path, &query)) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		fprintf(stderr, "This program expect $REQUEST_URI to be set.\n");
		error(500, NULL);
	}

	generate(do_route(path, query));

	return EXIT_SUCCESS;
}
