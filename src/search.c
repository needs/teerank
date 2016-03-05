#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "config.h"
#include "io.h"
#include "player.h"

int main(int argc, char **argv)
{
	DIR *dir = NULL;
	struct dirent *dp;
	static char path[PATH_MAX];
	static char hex[MAX_NAME_LENGTH];
	unsigned count = 0;

	load_config();

	if (argc != 2) {
		fprintf(stderr, "usage: %s <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";

	print_header(&CUSTOM_TAB, "Search results", argv[1]);

	/* TODO: Better message */
	if (strlen(argv[1]) >= MAX_NAME_LENGTH) {
		printf("No player found\n");
		print_footer();
		return EXIT_SUCCESS;
	} else if (strlen(argv[1]) < 3) {
		goto no_result;
	} else {
		string_to_hex(argv[1], hex);
	}

	if (snprintf(path, PATH_MAX, "%s/players", config.root) >= PATH_MAX) {
		fprintf(stderr, "Path to teerank database too long\n");
		return EXIT_FAILURE;
	}
	if (!(dir = opendir(path))) {
		fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
		return EXIT_FAILURE;
	}

	while ((dp = readdir(dir))) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strstr(dp->d_name, hex)) {
			struct player player;

			if (count == 0)
				printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");

			read_player(&player, dp->d_name);
			html_print_player(&player, 1);
			count++;
		}
	}

no_result:
	if (count > 0)
		printf("</tbody></table>");
	else
		printf("No players found");

	if (dir)
		closedir(dir);

	print_footer();

	return EXIT_SUCCESS;
}
