/*
 * From players with no history, add one history entry with the current
 * elo of the player and the current time.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include "config.h"
#include "player.h"

static void init_history(struct history *history, int elo, unsigned rank)
{
	assert(history != NULL);

	history->current.elo = elo;
	history->current.rank = rank;
	history->current.timestamp = time(NULL);
	history->length = 0;
}

static int old_read_player(struct player *player, char *name)
{
        static char path[PATH_MAX];
        int ret;
        FILE *file;

        assert(name != NULL);
        assert(player != NULL);
        assert(is_valid_hexname(name));

        ret = snprintf(path, PATH_MAX, "%s/players/%s", config.root, name);
        if (ret >= PATH_MAX) {
	        fprintf(stderr, "%s: Path too long\n", name);
	        return 0;
        }

        if (!(file = fopen(path, "r")))
	        return perror(path), 0;

        errno = 0;
        ret = fscanf(file, "%s %d %u\n", player->clan,
                     &player->elo, &player->rank);
        if (ret == EOF && errno != 0) {
                perror(path);
                goto fail;
        } else if (ret == EOF || ret == 0) {
                fprintf(stderr, "%s: Cannot match player clan\n", path);
                goto fail;
        } else if (ret == 1) {
                fprintf(stderr, "%s: Cannot match player elo\n", path);
                goto fail;
        } else if (ret == 2) {
                fprintf(stderr, "%s: Cannot match player rank\n", path);
                goto fail;
        }

        strcpy(player->name, name);
        init_history(&player->history, player->elo, player->rank);

        fclose(file);
        return 1;

fail:
        fclose(file);
        return 0;
}

int main(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *dp;
	int ret;
	static char path[PATH_MAX];

	load_config();

	ret = snprintf(path, PATH_MAX, "%s/players", config.root);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Path to long\n", config.root);
		return EXIT_FAILURE;
	}

	if (!(dir = opendir(path)))
		return perror(path), EXIT_FAILURE;

	while ((dp = readdir(dir))) {
		struct player player = PLAYER_ZERO;

		if (!is_valid_hexname(dp->d_name))
			continue;

		old_read_player(&player, dp->d_name);
		write_player(&player);
	}

	closedir(dir);

	return EXIT_SUCCESS;
}
