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
#include <limits.h>
#include <time.h>
#include <ctype.h>

#include "config.h"

#define NAME_LENGTH 17
#define HEXNAME_LENGTH 33

int is_valid_hexname(const char *hex)
{
	size_t length;

	assert(hex != NULL);

	length = strlen(hex);
	if (length >= HEXNAME_LENGTH)
		return 0;
	if (length % 2 == 1)
		return 0;

	while (isxdigit(*hex))
		hex++;
	return *hex == '\0';
}

struct history_entry {
	time_t timestamp;
	int elo;
	unsigned rank;
};

struct history {
	int is_full;

	unsigned buffer_length, length;

	struct history_entry current;

	struct history_entry *entries;
};

struct player {
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];

	struct history history;

	int elo;
	unsigned rank;

	struct player_delta *delta;

	unsigned short is_modified;

	short is_rankable;
};

static char *get_path(char *name)
{
	static char path[PATH_MAX];

	assert(name != NULL);

	if (snprintf(path, PATH_MAX, "%s/players/%s",
	             config.root, name) >= PATH_MAX) {
		fprintf(stderr, "%s: Path too long\n", config.root);
		return NULL;
	}

	return path;
}

static int write_history(FILE *file, const char *path, struct history *history)
{
	int ret;
	unsigned i;

	assert(file != NULL);
	assert(history != NULL);

	/* First write the current history entry */
	ret = fprintf(file, "current: %lu %d %u\n",
	              history->current.timestamp,
	              history->current.elo, history->current.rank);
	if (ret < 0)
		goto fail;

	/* Then comes the history */
	ret = fprintf(file, "history: %u entries", history->length);
	if (ret < 0)
		goto fail;

	for (i = 0; i < history->length; i++) {
		struct history_entry *entry = &history->entries[i];
		ret = fprintf(file, ", %lu %d %u",
		              entry->timestamp, entry->elo, entry->rank);
		if (ret < 0)
			goto fail;
	}
	fputc('\n', file);
	return 1;

fail:
	perror(path);
	return 0;
}

static int write_player(struct player *player)
{
	char *path;
	FILE *file;

	assert(player != NULL);
	assert(player->name[0] != '\0');

	if (!(path = get_path(player->name)))
		return 0;
	if (!(file = fopen(path, "w")))
		return perror(path), 0;

	if (fprintf(file, "%s\n", player->clan) < 0) {
		perror(path);
		goto fail;
	}

	if (!write_history(file, path, &player->history)) {
		perror(path);
		goto fail;
	}

	fclose(file);
	return 1;
fail:
	fclose(file);
	return 0;
}

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
        static char *path;
        int ret;
        FILE *file;

        assert(name != NULL);
        assert(player != NULL);
        assert(is_valid_hexname(name));

        if (!(path = get_path(name)))
	        return 0;

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

static const struct player PLAYER_ZERO;

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
