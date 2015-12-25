#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "config.h"

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
	FILE *file = NULL, *ranks_file;
	char path[PATH_MAX];
	int c;

	if (argc != 2) {
		fprintf(stderr, "usage: %s players_per_page\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!validate_players_per_page(argv[1], &players_per_page))
		return EXIT_FAILURE;

	sprintf(path, "%s/ranks", config.root);
	if (!(ranks_file = fopen(path, "r")))
		return perror(path), EXIT_FAILURE;

	/* First line contains the number of players */
	if (fscanf(ranks_file, "%u players ", &players_count) != 1) {
		fprintf(stderr, "Can't read the number of players\n");
		return EXIT_FAILURE;
	}

	goto start;
	while ((c = fgetc(ranks_file)) != EOF) {
		fputc(c, file);
		if (c == '\n') {
			lines++;

			if (lines % players_per_page == 0) {
				if (file)
					fclose(file);

			start:
				sprintf(path, "%s/pages/%u", config.root,
				        lines / players_per_page + 1);
				if (!(file = fopen(path, "w")))
					return perror(path), EXIT_FAILURE;

				/* Print page header */
				fprintf(file, "page %u/%u\n",
				        lines / players_per_page + 1,
				        players_count / players_per_page + 1);
			}
		}
	}

	if (file)
		fclose(file);
	fclose(ranks_file);

	return EXIT_SUCCESS;
}
