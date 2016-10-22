#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "page.h"

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

int dump(const char *path)
{
	FILE *file;
	int c;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return EXIT_NOT_FOUND;
		perror(path);
		return EXIT_FAILURE;
	}

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
	return EXIT_SUCCESS;
}
