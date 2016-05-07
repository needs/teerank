#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "player.h"
#include "config.h"
#include "elo.h"

/* Minimum time between two entries */
#define HISTORY_TIMEFRAME_LENGTH 3600

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

void hexname_to_name(const char *hex, char *str)
{
	assert(hex != NULL);
	assert(str != NULL);
	assert(hex != str);

	for (; hex[0] != '0' || hex[1] != '0'; hex += 2, str++) {
		char tmp[3] = { hex[0], hex[1], '\0' };
		*str = strtol(tmp, NULL, 16);
	}

	*str = '\0';
}

void name_to_hexname(const char *str, char *hex)
{
	assert(str != NULL);
	assert(hex != NULL);
	assert(str != hex);

	for (; *str; str++, hex += 2)
		sprintf(hex, "%2x", *(unsigned char*)str);
	strcpy(hex, "00");
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

/*
 * Set all player's fields to meaningless values suitable for printing.
 */
static void reset_player(struct player *player)
{
	strcpy(player->clan, "00");
	player->history.current.rank = player->rank = INVALID_RANK;
	player->history.current.elo = player->elo = INVALID_ELO;
	player->is_modified = 0;

	player->delta = NULL;

	player->history.length = 0;
}

/* player must have been reseted */
static void new_player(struct player *player, char *name)
{
	struct history *history;

	assert(name != NULL);
	assert(player != NULL);

	player->history.current.elo = player->elo = DEFAULT_ELO;
	player->history.current.rank = player->rank = INVALID_RANK;
	player->history.current.timestamp = time(NULL);
	history->length = 0;

	player->is_rankable = 0;
	player->is_modified = IS_MODIFIED_CREATED;
}

static int resize_history(struct history *history, unsigned length)
{
	struct history_entry *entries;
	const unsigned OFFSET = 1024;
	unsigned buffer_length;

	assert(history != NULL);
	assert(history->length <= history->buffer_length);
	assert(history->buffer_length != 0 || history->entries == NULL);

	if (length <= history->buffer_length)
		return 1;

	buffer_length = length - (length % OFFSET) + OFFSET;
	entries = realloc(history->entries, buffer_length * sizeof(*entries));
	if (!entries) {
		perror("realloc(history)");
		return 0;
	}

	history->entries = entries;
	history->buffer_length = buffer_length;

	return 1;
}

static int read_full_history(FILE *file, const char *path,
                             struct history *history)
{
	unsigned length, i;
	int ret;

	assert(history != NULL);

	errno = 0;
	ret = fscanf(file, " history: %u entries", &length);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match history length\n", path), 0;

	if (!resize_history(history, length))
		return 0;

	for (i = 0; i < length; i++) {
		struct history_entry *entry = &history->entries[i];

		errno = 0;
		ret = fscanf(file, " , %lu %d %u",
		             &entry->timestamp, &entry->elo, &entry->rank);
		if (ret == EOF && errno != 0)
			perror(path);
		else if (ret == EOF || ret == 0)
			fprintf(stderr, "%s: entry %u: Cannot match timestamp\n", path, i);
		else if (ret == 1)
			fprintf(stderr, "%s: entry %u: Cannot match elo\n", path, i);
		else if (ret == 2)
			fprintf(stderr, "%s: entry %u: Cannot match rank\n", path, i);
		else
			continue;
		break;
	}

	/* 'i' contains the number of entries succesfully read */
	history->length = i;
	return i == length;
}

static int read_history(FILE *file, const char *path,
                        struct player *player,
                        int full_history)
{
	struct history *history;
	int ret;

	assert(file != NULL);
	assert(player != NULL);

	history = &player->history;

	/* First comes the current entry */
	errno = 0;
	ret = fscanf(file, " current: %lu %d %u",
	             &history->current.timestamp, &history->current.elo, &history->current.rank);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: current entry: Cannot match timestamp\n", path), 0;
	else if (ret == 1)
		return fprintf(stderr, "%s: current entry: Cannot match elo\n", path), 0;
	else if (ret == 2)
		return fprintf(stderr, "%s: current entry: Cannot match rank\n", path), 0;

	player->elo = history->current.elo;
	player->rank = history->current.rank;

	/* Then comes the history, to be read only if needed */
	if (full_history)
		return read_full_history(file, path, history);
	else
		return 1;
}

int read_player(struct player *player, char *name, int full_history)
{
	char *path;
	int ret;
	FILE *file;

	assert(name != NULL);
	assert(player != NULL);
	assert(is_valid_hexname(name));

	/*
	 * Reset player sets every fields to a 'no value' state, invalid for
	 * advanced use but still suitable for printing the player.
	 *
	 * That mean failing at every points of the function will leaves
	 * us with a player suitable for printing, as expected.
	 */
	reset_player(player);
	strcpy(player->name, name);

	if (!(path = get_path(name)))
		return 0;
	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return new_player(player, name), 1;
		else
			return perror(path), 0;
	}

	errno = 0;
	ret = fscanf(file, "%s\n", player->clan);
	if (ret == EOF && errno != 0) {
		perror(path);
		goto fail;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match player clan\n", path);
		goto fail;
	}

	if (!read_history(file, path, player, full_history))
		goto fail;

	fclose(file);
	return 1;

fail:
	fclose(file);
	return 0;
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

int write_player(struct player *player)
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

static time_t timeframe(time_t t)
{
	return t - (t % HISTORY_TIMEFRAME_LENGTH);
}

static time_t next_timeframe(time_t t)
{
	return timeframe(t) + HISTORY_TIMEFRAME_LENGTH;
}

/*
 * Copy the current enrty at the end of the history
 */
static int archive_current(struct history *history)
{
	struct history_entry *entry;

	if (!resize_history(history, history->length + 1))
		return 0;

	entry = &history->entries[history->length];
	history->length++;

	*entry = history->current;
	entry->timestamp = next_timeframe(entry->timestamp);
	return 1;
}

/*
 * Update player ELO points, update history if necessary.
 */
int update_elo(struct player *player, int elo)
{
	struct history *history;
	time_t now;

	assert(player != NULL);

	history = &player->history;
	now = time(NULL);

	/*
	 * The current history entry is archived when it's timeframe is
	 * lower than the actual one.
	 */
	fprintf(stderr, "%32s: now tf = %lu, current tf = %lu (diff = %lu)\n",
	        player->name, timeframe(now), timeframe(history->current.timestamp), timeframe(now) - timeframe(history->current.timestamp));

	if (timeframe(history->current.timestamp) < timeframe(now))
		if (!archive_current(history))
			return 0;

	history->current.elo = elo;
	history->current.timestamp = now;
	player->elo = elo;
	player->is_modified |= IS_MODIFIED_ELO;

	return 1;
}

struct player *add_player(struct player_array *array, struct player *player)
{
	const unsigned OFFSET = 1024;
	struct player *cell;

	assert(array  != NULL);
	assert(player != NULL);

	if (array->length % OFFSET == 0) {
		struct player *tmp;

		tmp = realloc(array->players, (array->length + OFFSET) * sizeof(*array->players));
		if (!tmp)
			return perror("Allocating player array"), NULL;
		array->players = tmp;
	}

	cell = &array->players[array->length++];
	*cell = *player;
	return cell;
}
