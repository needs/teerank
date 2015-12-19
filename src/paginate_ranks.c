#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

static int validate_path_format(const char *fmt)
{
	assert(fmt != NULL);

	/* Simple: path format should contain one and only one "%u" */
	if (!(fmt = strstr(fmt, "%u")))
		return fprintf(stderr, "path_format: Should contain one \"%%u\"\n"), 0;
	if ((fmt = strstr(fmt + 1, "%u")))
		return fprintf(stderr, "path_format: Should contain only one \"%%u\"\n"), 0;
	return 1;
}

static int validate_players_per_page(const char *str, unsigned *ret)
{
	long tmp;
	char *end;

	tmp = strtol(str, &end, 10);
	if (*end != '\0')
		return fprintf(stderr, "players_per_page: Is not a valid number\n"), 0;
	if (tmp <= 0)
		return fprintf(stderr, "players_per_page: Should be a positive number\n"), 0;
	if (tmp > UINT_MAX)
		return fprintf(stderr, "players_per_page: Should lower than %u\n", UINT_MAX), 0;

	*ret = (unsigned)tmp;
	return 1;
}

int main(int argc, char *argv[])
{
	unsigned players_per_page, players_count, lines = 0;
	FILE *file = NULL;
	int c;

	if (argc < 3) {
		fprintf(stderr, "usage: %s players_per_page path_format\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!validate_players_per_page(argv[1], &players_per_page))
		return EXIT_FAILURE;
	if (!validate_path_format(argv[2]))
		return EXIT_FAILURE;

	/* First line contains the number of players */
	if (scanf("%u players ", &players_count) != 1) {
		fprintf(stderr, "Can't read the number of players\n");
		return EXIT_FAILURE;
	}

	goto start;
	while ((c = getchar()) != EOF) {
		fputc(c, file);
		if (c == '\n') {
			lines++;

			if (lines % players_per_page == 0) {
				static char path[PATH_MAX];
				if (file)
					fclose(file);

			start:
				sprintf(path, argv[2], lines / players_per_page + 1);
				if (!(file = fopen(path, "w")))
					return perror(path), EXIT_FAILURE;

				/* Print page header */
				fprintf(file, "page %u/%u\n",
				        lines / players_per_page + 1,
				        players_count / players_per_page + 1);
			}
		}
	}

	return EXIT_SUCCESS;
}
