/*
 * Old format did not have any kind of historic.  This program does
 * initialize every necessary historic with values available.  Hence
 * resulting historics will always have one single record.
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
#include <unistd.h>

#include "4-to-5.h"
#include "config.h"
#include "player.h"
#include "historic.h"

static int read_old_player(struct player *player, char *name)
{
	static char path[PATH_MAX];
        FILE *file = NULL;
        int ret;
        struct player_record rec;

        assert(name != NULL);
        assert(player != NULL);
        assert(is_valid_hexname(name));

	if (snprintf(path, PATH_MAX, "%s/players/%s",
	             config.root, name) >= PATH_MAX) {
		fprintf(stderr, "%s: Path too long\n", config.root);
		goto fail;
	}

	if (!(file = fopen(path, "r"))) {
		perror(path);
		goto fail;
	}

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

        create_historic(&player->hist);

        rec.elo = player->elo;
        rec.rank = player->rank;
        append_record(&player->hist, &rec);

        return 1;

fail:
        if (file)
	        fclose(file);
        return 0;
}

static void remove_player(const char *name)
{
	char path[PATH_MAX];
	int ret;

	ret = snprintf(path, PATH_MAX, "%s/players/%s", config.root, name);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		return;
	}

	ret = unlink(path);

	/*
	 * Unlikely but still possible: someone else may have removed
	 * player right before us, ignore it.
	 */
	if (ret == -1 && errno != ENOENT)
		perror(path);
}

void upgrade_players(void)
{
	DIR *dir;
	struct dirent *dp;
	int ret;
	static char path[PATH_MAX];
	struct player player;

	ret = snprintf(path, PATH_MAX, "%s/players", config.root);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Path to long\n", config.root);
		exit(EXIT_FAILURE);
	}

	dir = opendir(path);
	if (!dir) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	init_player(&player);
	while ((dp = readdir(dir))) {

		if (!is_valid_hexname(dp->d_name))
			continue;

		if (!read_old_player(&player, dp->d_name)) {
			remove_player(dp->d_name);
			continue;
		}

		write_player(&player);
	}

	closedir(dir);
}
