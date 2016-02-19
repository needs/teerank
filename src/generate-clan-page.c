#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "io.h"
#include "config.h"
#include "player.h"

static char path[PATH_MAX];

struct player_array {
	unsigned length;
	struct player *players;
};

static void add_player(struct player_array *array, struct player *player)
{
	static const unsigned OFFSET = 1024;

	assert(array != NULL);
	assert(player != NULL);

	if (array->length % OFFSET == 0) {
		void *tmp = realloc(array->players,
		                    (array->length + OFFSET) * sizeof(*player));
		if (!tmp)
			return perror("Reallocating players array");
		array->players = tmp;
	}

	array->players[array->length++] = *player;
}

static const struct player_array PLAYER_ARRAY_ZERO;

static int cmp_player(const void *p1, const void *p2)
{
	const struct player *a = p1, *b = p2;

	/* We want them in reverse order */
	if (b->rank > a->rank)
		return -1;
	if (b->rank < a->rank)
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	FILE *file;
	char name[MAX_NAME_LENGTH], clan[MAX_NAME_LENGTH];
	struct player_array array = PLAYER_ARRAY_ZERO;
	unsigned i;

	load_config();
	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Load players */

	sprintf(path, "%s/clans/%s", config.root, argv[1]);
	if (!(file = fopen(path, "r")))
		return perror(path), EXIT_FAILURE;

	while (fscanf(file, " %s", name) == 1) {
		struct player player;

		if (!read_player(&player, name))
			strcpy(player.clan, argv[1]);
		add_player(&array, &player);
	}

	fclose(file);

	/* Sort players */
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	/* Eventually, print them */
	hex_to_string(argv[1], clan);
	print_header(CTF_TAB, NULL);
	printf("<h2>%s</h2>\n", clan);
	printf("<p>%u member(s)</p>\n", array.length);
	printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead>\n<tbody>\n");

	for (i = 0; i < array.length; i++)
		html_print_player(&array.players[i], 0);

	printf("</tbody></table>\n");
	print_footer();

	return EXIT_SUCCESS;
}
