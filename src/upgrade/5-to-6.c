/*
 * History format have change once again.  This program read the old historic
 * while converting them to the new data structure and then write it.
 *
 * The convertion is kinda tricky because with the old format only one historic
 * hold both rank and elo values over time, where the new format have two
 * distinct historic.
 *
 * Let's note that the old data structure was named "history" where the new one
 * is "historic".
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include "config.h"
#include "player.h"
#include "historic.h"

static int alloc_historic(struct historic *hist, unsigned length)
{
	void *tmp;

	if (length <= hist->length)
		return 1;

	tmp = realloc(hist->records, length * sizeof(*hist->records));
	if (!tmp)
		return perror("realloc(records)"), 0;
	hist->records = tmp;

	tmp = realloc(hist->data, length * hist->data_size);
	if (!tmp)
		return perror("realloc(data)"), 0;
	hist->data = tmp;

	hist->length = length;
	return 1;
}

static int read_remaining_records(FILE *file, const char *path,
                                  struct player *player)
{
	unsigned length, i;
	int ret;

	assert(player != NULL);

	errno = 0;
	ret = fscanf(file, " history: %u entries", &length);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match history length\n", path), 0;

	/*
	 * Old historic have last_record stored in a separate field, hence
	 * length do not count it.  However new historic treats last record
	 * as a normal record so we must count it
	 * */
	length += 1;

	if (!alloc_historic(&player->elo_historic, length))
		return 0;
	if (!alloc_historic(&player->rank_historic, length))
		return 0;

	player->elo_historic.nrecords = length;
	player->rank_historic.nrecords = length;

	for (i = 0; i < length - 1; i++) {
		struct record *elo_record = &player->elo_historic.records[i];
		struct record *rank_record = &player->rank_historic.records[i];

		time_t timestamp;
		int elo;
		unsigned rank;

		errno = 0;
		ret = fscanf(file, " , %lu %d %u", &timestamp, &elo, &rank);
		if (ret == EOF && errno != 0)
			return perror(path), 0;
		else if (ret == EOF || ret == 0)
			return fprintf(stderr, "%s: entry %u: Cannot match timestamp\n", path, i), 0;
		else if (ret == 1)
			return fprintf(stderr, "%s: entry %u: Cannot match elo\n", path, i), 0;
		else if (ret == 2)
			return fprintf(stderr, "%s: entry %u: Cannot match rank\n", path, i), 0;

		/* Set record timestamp */
		elo_record->time = timestamp;
		rank_record->time = timestamp;

		/* Set data values */
		*(int*)get_record_data(&player->elo_historic, elo_record) = elo;
		*(unsigned*)get_record_data(&player->rank_historic, rank_record) = rank;
	}

	return 1;
}

static int read_full_historic(FILE *file, const char *path,
                              struct player *player)
{
	int ret;

	struct record *last;
	time_t timestamp;
	int elo;
	unsigned rank;

	assert(file != NULL);
	assert(player != NULL);

	init_historic(&player->elo_historic, sizeof(player->elo), &player->elo);
	init_historic(&player->rank_historic, sizeof(player->rank), &player->rank);

	/* First comes the current entry (= last record) */
	errno = 0;
	ret = fscanf(file, " current: %lu %d %u", &timestamp, &elo, &rank);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: current entry: Cannot match timestamp\n", path), 0;
	else if (ret == 1)
		return fprintf(stderr, "%s: current entry: Cannot match elo\n", path), 0;
	else if (ret == 2)
		return fprintf(stderr, "%s: current entry: Cannot match rank\n", path), 0;

	/*
	 * Read remaining records before storing last record because we don't
	 * know the number of records and hence we can't store the last record
	 * at the end of an array of unknown size.
	 */

	if (!read_remaining_records(file, path, player))
		return 0;

	/* Now last_record() will return meaningful values */

	last = last_record(&player->elo_historic);
	last->time = timestamp;
	*(int*)get_record_data(&player->elo_historic, last) = elo;

	last = last_record(&player->rank_historic);
	last->time = timestamp;
	*(unsigned*)get_record_data(&player->rank_historic, last) = rank;

	/* first_record() won't return NULL because there is at least one record */

	player->elo_historic.epoch = first_record(&player->elo_historic)->time;
	player->rank_historic.epoch = first_record(&player->rank_historic)->time;

	return 1;
}

static char *get_path(char *name)
{
	static char path[PATH_MAX];

	assert(name != NULL);

	if (snprintf(path, PATH_MAX, "%s/players/%s",
	             config.root, name) >= PATH_MAX)
		return NULL;
	return path;
}

int old_read_player(struct player *player, char *name)
{
	char *path;
	int ret;
	FILE *file;

	assert(name != NULL);
	assert(player != NULL);
	assert(is_valid_hexname(name));

	strcpy(player->name, name);

	if (!(path = get_path(name)))
		return 0;
	if (!(file = fopen(path, "r")))
		return perror(path), 0;

	errno = 0;
	ret = fscanf(file, "%s\n", player->clan);
	if (ret == EOF && errno != 0) {
		perror(path);
		goto fail;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match player clan\n", path);
		goto fail;
	}

	if (!read_full_historic(file, path, player))
		goto fail;

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
