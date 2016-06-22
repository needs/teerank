/*
 * Old format did not have any kind of historic.  This program does
 * initialize every necessary historic with values available.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#include "config.h"
#include "player.h"
#include "historic.h"

static int read_old_player(struct player *player, char *name)
{
	const time_t HOUR = 60 * 60;
	static char path[PATH_MAX];
        FILE *file;
        int ret;

        assert(name != NULL);
        assert(player != NULL);
        assert(is_valid_hexname(name));

	if (snprintf(path, PATH_MAX, "%s/players/%s",
	             config.root, name) >= PATH_MAX) {
		fprintf(stderr, "%s: Path too long\n", config.root);
		return 0;
	}

        if (!(file = fopen(path, "r")))
	        return perror(path), 0;

        errno = 0;
        ret = fscanf(file, "%s %d %u\n",
                     player->clan, &player->elo, &player->rank);
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

        fclose(file);

        strcpy(player->name, name);
        init_historic(&player->elo_historic, sizeof(player->elo), 0);
        init_historic(&player->hourly_rank,  sizeof(player->rank), HOUR);
        init_historic(&player->daily_rank,   sizeof(player->rank), 24 * HOUR);
        init_historic(&player->monthly_rank, sizeof(player->rank), 30 * 24 * HOUR);

        /*
         * Error message refers to "init" despite initialization phase is
         * finish because technically an empty historic is not considered
         * as initialized.
         */

        if (!append_record(&player->elo_historic, &player->elo)) {
	        fprintf(stderr, "%s: Cannot init elo historic\n", path);
	        return 0;
        }
        if (!append_record(&player->hourly_rank, &player->rank)) {
	        fprintf(stderr, "%s: Cannot init hourly rank historic\n", path);
	        return 0;
        }
        if (!append_record(&player->daily_rank, &player->rank)) {
	        fprintf(stderr, "%s: Cannot init daily rank historic\n", path);
	        return 0;
        }
        if (!append_record(&player->monthly_rank, &player->rank)) {
	        fprintf(stderr, "%s: Cannot init monthly rank historic\n", path);
	        return 0;
        }

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

	dir = opendir(path);
	if (!dir)
		return perror(path), EXIT_FAILURE;

	while ((dp = readdir(dir))) {
		struct player player = PLAYER_ZERO;

		if (!is_valid_hexname(dp->d_name))
			continue;

		if (!read_old_player(&player, dp->d_name))
			continue;

		write_player(&player);
	}

	closedir(dir);

	return EXIT_SUCCESS;
}
