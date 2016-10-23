#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "player.h"
#include "page.h"

static int dump_n_fields(FILE *file, unsigned n)
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

int main_json_player(int argc, char **argv)
{
	char path[PATH_MAX];
	int full;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <player_name> full|short\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[2], "full") == 0)
		full = 1;
	else if (strcmp(argv[2], "short") == 0)
		full = 0;
	else {
		fprintf(stderr, "%s: Should be either \"full\" or \"short\"\n", argv[2]);
		return EXIT_FAILURE;
	}

	if (!is_valid_hexname(argv[1]))
		return EXIT_NOT_FOUND;

	if (!dbpath(path, PATH_MAX, "players/%s", argv[1]))
		return EXIT_FAILURE;

	if (full) {
		return dump(path);
	} else {
		FILE *file;
		int success;

		if (!(file = fopen(path, "r"))) {
			if (errno == ENOENT)
				return EXIT_NOT_FOUND;
			else
				return EXIT_FAILURE;
		}

		success = dump_n_fields(file, 7);
		putchar('}');
		fclose(file);

		if (success)
			return EXIT_SUCCESS;
		else
			return EXIT_FAILURE;
	}

}
