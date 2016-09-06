#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

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

int dump_n_fields(FILE *file, unsigned n)
{
	/*
	 * Each time we have a coma that is not in a string, increase
	 * field count.  Closing brace and closing bracket also increase
	 * field count.
	 */
	int c, instring = 0, forcenext = 0;

	goto start;
	while (n) {
		putchar(c);

	start:
		c = fgetc(file);

		if (c == EOF)
			return 0;

		if (forcenext)
			forcenext = 0;
		else if (instring && c == '\\')
			forcenext = 1;
		else if (!instring && c == '\"')
			instring = 1;
		else if (instring && c == '\"')
			instring = 0;
		else if (c == ',' || c == '}' || c == ']')
			n--;
	}

	return 1;
}
