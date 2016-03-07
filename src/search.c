#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "config.h"
#include "io.h"
#include "player.h"

static void search(char *hex, struct player_array *array)
{
	DIR *dir;
	struct dirent *dp;
	static char path[PATH_MAX];

	if (snprintf(path, PATH_MAX, "%s/players", config.root) >= PATH_MAX) {
		fprintf(stderr, "Path to teerank database too long\n");
		exit(EXIT_FAILURE);
	}

	if (!(dir = opendir(path))) {
		fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while ((dp = readdir(dir))) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strstr(dp->d_name, hex)) {
			struct player player;

			if (!read_player(&player, dp->d_name))
				continue;
			add_player(array, &player);
		}
	}

	closedir(dir);
}

static struct player_array PLAYER_ARRAY_ZERO;

int main(int argc, char **argv)
{
	static char hex[MAX_NAME_LENGTH];
	struct player_array array = PLAYER_ARRAY_ZERO;
	int i;

	load_config();

	if (argc != 2) {
		fprintf(stderr, "usage: %s <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strlen(argv[1]) < MAX_NAME_LENGTH && strlen(argv[1]) >= 3) {
		string_to_hex(argv[1], hex);
		search(hex, &array);
	}

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";
	print_header(&CUSTOM_TAB, "Search results", argv[1]);

	if (array.length == 0) {
		printf("No players found");
	} else {
		printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");
		for (i = 0; i < array.length && i < 100; i++)
			html_print_player(&array.players[i], 1);
		printf("</tbody></table>");
	}

	print_footer();

	return EXIT_SUCCESS;
}
