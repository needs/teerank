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

#include "4-to-5.h"
#include "config.h"
#include "player.h"
#include "historic.h"

/*
 * Internally historics record buffer and data buffer are allocated on
 * the heap using malloc().  However we know that only a single record
 * will be stored in those historics.  hence we will manually set
 * those buffers to static storage.
 *
 * It's definitively a hack but it will speed up things and will not
 * require alloc_historic() from historic.c to be exposed in the
 * header just because we exclusively need it here.
 *
 * It basically speed up the process because malloc() is expensive.
 * However since allocated buffers are reused when using the same
 * struct player for different players, it is not that much of a gain.
 */
static void static_alloc_historics(struct player *player)
{
	static struct record records[1];
	static struct player_record record_data[1];

	player->hist.records = records;
	player->hist.nrecords = 0;
	player->hist.data = &record_data;
	player->hist.length = 1;
	player->hist.epoch = time(NULL);
}

static int read_old_player(struct player *player, char *name)
{
	static char path[PATH_MAX];
        FILE *file;
        int ret;
        struct player_record rec;

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

        static_alloc_historics(player);

        rec.elo = player->elo;
        rec.rank = player->rank;

        /*
         * Error message refers to "init" despite initialization phase is
         * finish because technically an empty historic is not considered
         * as initialized.
         */
        if (!append_record(&player->hist, &rec)) {
	        fprintf(stderr, "%s: Cannot init player historic\n", path);
	        return 0;
        }

        return 1;

fail:
        fclose(file);
        return 0;
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

		if (!read_old_player(&player, dp->d_name))
			continue;

		write_player(&player);
	}

	closedir(dir);
}
