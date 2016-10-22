#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "server.h"
#include "page.h"

int main_json_server(int argc, char **argv)
{
	char path[PATH_MAX];
	char *ip, *port;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <server_ip>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!parse_addr(argv[1], &ip, &port))
		return EXIT_NOT_FOUND;

	if (!dbpath(path, PATH_MAX, "servers/%s", server_filename(ip, port)))
		return EXIT_FAILURE;

	return dump(path);
}
