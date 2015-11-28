#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "io.h"

static char path[PATH_MAX];

static char *players_directory;

/*
 * Strictly speaking, INT_MIN is still a valid Elo.  But it just impossible
 * to get it.  And even if a player got it somehow, it will messup the whole
 * system because of underflow, so INT_MIN is safe to use.
 */
#define UNKNOWN_ELO INT_MIN

static int load_elo(const char *name)
{
	int ret, elo;

	assert(name != NULL);

	sprintf(path, "%s/%s/%s", players_directory, name, "elo");

	/* Failing at reading Elo is not fatal, we will just output '?' */
	ret = read_file(path, "%d", &elo);
	if (ret == 1)
		return elo;
	else if (ret == -1)
		perror(path);
	else
		fprintf(stderr, "%s: Cannot match Elo\n", path);

	return UNKNOWN_ELO;
}

static int extract_clan_string(char *clan_directory, char *clan)
{
	char *tmp;

	assert(clan_directory != NULL);
	assert(clan != NULL);

	tmp = basename(clan_directory);
	if (strlen(tmp) == MAX_NAME_LENGTH - 1) {
		fprintf(stderr, "%s: Invalid clan directory\n", clan_directory);
		return 0;
	}

	hex_to_string(tmp, clan);
	return 1;
}

static void print_file(char *path)
{
	FILE *file = NULL;
	int c;

	if (!(file = fopen(path, "r")))
		exit(EXIT_FAILURE);

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
}

int main(int argc, char **argv)
{
	FILE *file;
	char name[MAX_NAME_LENGTH], clan[MAX_NAME_LENGTH];

	if (argc != 3) {
		fprintf(stderr, "usage: %s <clan_directory> <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	players_directory = argv[2];

	if (!extract_clan_string(argv[1], clan))
		return EXIT_FAILURE;

	sprintf(path, "%s/%s", argv[1], "members");
	if (!(file = fopen(path, "r")))
		return perror(path), EXIT_FAILURE;

	print_file("html/header_clan.inc.html");

	printf("<h2>%s</h2>\n", clan);
	printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead>\n<tbody>");

	while (fscanf(file, " %s", name) == 1) {
		int elo;
		char _name[MAX_NAME_LENGTH];

		elo = load_elo(name);
		hex_to_string(name, _name);

		printf("<tr><td></td>");
		printf("<td>%s</td><td>%s</td>", _name, clan);
		if (elo == UNKNOWN_ELO)
			printf("<td>?</td>");
		else
			printf("<td>%d</td>", elo);
		printf("</tr>\n");
	}

	printf("</tbody></table>");
	print_file("html/footer_clan.inc.html");

	fclose(file);

	return EXIT_SUCCESS;
}
