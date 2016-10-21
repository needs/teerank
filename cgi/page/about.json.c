#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"

static int dump(const char *path)
{
	FILE *file;
	int c;

	if (!(file = fopen(path, "r"))) {
		perror(path);
		return EXIT_FAILURE;
	}

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
	return EXIT_SUCCESS;
}

int main_json_about(int argc, char **argv)
{
	char path[PATH_MAX];

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!dbpath(path, PATH_MAX, "info"))
		return EXIT_FAILURE;

	return dump(path);
}
