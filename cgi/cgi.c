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
#include <stdbool.h>
#include <setjmp.h>

#include "teerank.h"
#include "html.h"
#include "cgi.h"

static jmp_buf errenv;
static char errmsg[1024];

struct cgi_config cgi_config = {
	"teerank.com", "80"
};

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
	printf("<h1>%i %s</h1>\n", code, reason_phrase(code));

	if (*errmsg)
		printf("<p>%s</p>", errmsg);
}

void error(int code, char *fmt, ...)
{
	if (fmt) {
		va_list ap;

		va_start(ap, fmt);
		vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
		va_end(ap);

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	} else {
		*errmsg = 0;
	}

	longjmp(errenv, code);
}

/*
 * FIXME! redirect() is now called within the page, and their is no way
 * for a page to forward specific headers in HTTP response.  So this
 * function as it is will not work as intented.
 */
static void redirect(char *url)
{
	assert(url != NULL);

	printf("Status: %d %s\n", 301, reason_phrase(301));
	printf("Location: http://%s%s\n", cgi_config.domain, url);
	putchar('\n');
}

unsigned parse_pnum(char *str)
{
	long ret;

	errno = 0;
	ret = strtol(str, NULL, 10);
	if (ret == 0 && errno != 0)
		error(400, "%s: %s", str, strerror(errno));

	/*
	 * Page numbers are unsigned but strtol() returns a long, so we
	 * need to make sure our page number fit into an unsigned.
	 */
	if (ret < 1)
		error(400, "%s: Must be positive", str);
	else if (ret > UINT_MAX)
		error(400, "%s: Must be lower than %u", str, UINT_MAX);

	return ret;
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
static void generate_html_teerank2_player_list(struct url *url)
{
	/* FIXME! Semantic changed and redirect() will not work as
	 * intended */
	redirect(URL("/players?p=%s", url->dirs[url->ndirs - 1]));
}

/* URLs for player graph looked like "/players/<hexname>/elo+rank.svg" */
static void generate_svg_teerank3_graph(struct url *url)
{
	/* FIXME! Semantic changed and redirect() will not work as
	 * intended */
	redirect(URL("/player/historic.svg?name=%s", convert_hexname(url->dirs[url->ndirs - 2])));
}

/*
 * It is a special route name used to match everything.  We can't use
 * NULL for that because it's already used for empty names.
 */
#define _ (char*)1

/*
 * The routes tree refers to pages "generate()" callbacks.  So we need to
 * declare them before buliding the tree.
 */
#define DIR(name)
#define END()
#define HTML(name, func)                                                \
	void generate_html_##func(struct url *url);
#define JSON(name, func)                                                \
	void generate_json_##func(struct url *url);
#define TXT(name, func)                                                 \
	void generate_txt_##func(struct url *url);
#define XML(name, func)                                                 \
	void generate_xml_##func(struct url *url);
#define SVG(name, func)                                                 \
	void generate_svg_##func(struct url *url);

#include "routes.def"

struct route;
struct route {
	const char *const name, *const ext;
	const char *const content_type;

	void (*const generate)(struct url *url);

	struct route *const routes;
};

/*
 * Build the root tree given data in "routes.def".
 */

#define DIR(name)                                                       \
	{ name, NULL, NULL, NULL, (struct route[]) {
#define END()                                                           \
	{ 0 } } },

#define HTML(name, func)                                                \
	{ name, "html", "text/html", generate_html_##func },            \
	{ name, NULL, "text/html", generate_html_##func },
#define JSON(name, func)                                                \
	{ name, "json", "text/json", generate_json_##func },
#define TXT(name, func)                                                 \
	{ name, "txt", "text/plain", generate_txt_##func },
#define XML(name, func)                                                 \
	{ name, "xml", "text/xml", generate_xml_##func },
#define SVG(name, func)                                                 \
	{ name, "svg", "image/svg+xml", generate_svg_##func },

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
		return route->generate ? samestr(route->ext, ext) : true;
	else
		return false;
}

static bool is_valid_route(struct route *route)
{
	return route && (route->generate || route->routes);
}

/* Find the first child route matching the given name */
static struct route *find_child_route(
	struct route *route, char *name, char *ext)
{
	struct route *r;

	for (r = route->routes; is_valid_route(r); r++)
		if (route_match(r, name, ext))
			return r;

	return NULL;
}

/* Find a leaf route (if any) matching the given URL */
static struct route *find_route(struct url *url)
{
	struct route *route = &root;
	unsigned i;

	for (i = 0; i < url->ndirs && route; i++)
		route = find_child_route(route, url->dirs[i], url->ext);

	if (route && !route->generate)
		route = find_child_route(route, NULL, url->ext);

	if (!route || !route->generate)
		error(404, NULL);

	return route;
}

static void load_path_and_query(char **_path, char **_query)
{
	static char pathbuf[1024], querybuf[1024];
	char *path, *query;

	path = getenv("PATH_INFO");
	query = getenv("QUERY_STRING");

	/*
	 * Even though PATH_INFO is the standard way, some webservers,
	 * like nginx, don't define by default this environment
	 * variable.  In such cases, we automatically fallback.
	 */
	if (!path)
		path = getenv("DOCUMENT_URI");

	if (!path)
		error(500, "$PATH_INFO or $DOCUMENT_URI not set.");

	/*
	 * Query string is optional, although I believe it's still
	 * defined when empty.
	 */
	if (!query)
		query = "";

	/*
	 * Both path and query string will be modified when they will be
	 * parsed.  Copy them to writable buffers, as buffers returned
	 * by getenv() are only readable.
	 */
	snprintf(pathbuf, sizeof(pathbuf), "%s", path);
	snprintf(querybuf, sizeof(querybuf), "%s", query);

	*_path = pathbuf;
	*_query = querybuf;
}

/* Load extra environment variables set by the webserver */
static void init_cgi(void)
{
	const char *fmt;

	cgi_config.name = getenv("SERVER_NAME");
	cgi_config.port = getenv("SERVER_PORT");

	/*
	 * Default to localhost when server name is undefined.  This
	 * should happen when testing CGI in the command line.
	 */
	if (!cgi_config.name)
		cgi_config.name = "localhost";
	if (!cgi_config.port)
		cgi_config.port = "80";

	if (strcmp(cgi_config.port, "80") == 0)
		cgi_config.port = NULL;

	if (cgi_config.port)
		fmt = "%s:%s";
	else
		fmt = "%s";

	/*
	 * Buffer for full server domain should be big enough for any
	 * practical domains.  Hence we don't check return value of
	 * snprintf().
	 */
	snprintf(
		cgi_config.domain, sizeof(cgi_config.domain), fmt,
		cgi_config.name, cgi_config.port);
}

static struct url parse_url(char *uri, char *query)
{
	struct url url = {0};
	char *dir, *name;
	unsigned i;

	for (dir = strtok(uri, "/"); dir; dir = strtok(NULL, "/")) {
		if (url.ndirs == MAX_DIRS)
			error(414, NULL);

		url_decode(dir);
		url.dirs[url.ndirs] = dir;
		url.ndirs++;
	}

	if (url.ndirs) {
		if ((url.ext = strrchr(url.dirs[url.ndirs-1], '.'))) {
			*url.ext = '\0';
			url.ext++;
		}
	}

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

int main(int argc, char **argv)
{
	char *path, *query;
	struct url url;
	struct route *route;
	int ret;

	/*
	 * We want to use read only mode to prevent any security exploit
	 * to be able to write the database.
	 */
	init_teerank(READ_ONLY);
	init_cgi();

	reset_output();

	if ((ret = setjmp(errenv))) {
		print_error(ret);
		return EXIT_FAILURE;
	}

	load_path_and_query(&path, &query);
	url = parse_url(path, query);
	route = find_route(&url);
	route->generate(&url);

	printf("Content-Type: %s\n", route->content_type);
	printf("\n");
	print_output();

	return EXIT_SUCCESS;
}
