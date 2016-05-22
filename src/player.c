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

void hexname_to_name(const char *hex, char *name)
{
	assert(hex != NULL);
	assert(name != NULL);
	assert(hex != name);

	for (; hex[0] != '0' || hex[1] != '0'; hex += 2, name++) {
		char tmp[3] = { hex[0], hex[1], '\0' };
		*name = strtol(tmp, NULL, 16);
	}

	*name = '\0';
}

void name_to_hexname(const char *name, char *hex)
{
	assert(name != NULL);
	assert(hex != NULL);
	assert(name != hex);

	for (; *name; name++, hex += 2)
		sprintf(hex, "%2x", *(unsigned char*)name);
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
static void reset_player(struct player *player, const char *name)
{
	strcpy(player->name, name);
	strcpy(player->clan, "00");
	player->elo = INVALID_ELO;
	player->rank = INVALID_RANK;
	player->is_modified = 0;

	player->delta = NULL;

	init_historic(&player->elo_historic, sizeof(player->elo));
	init_historic(&player->rank_historic, sizeof(player->rank));
}

/* player must have been reseted */
static int new_player(struct player *player)
{
	assert(player != NULL);

	if (!append_record(&player->elo_historic, &DEFAULT_ELO))
		return 0;

	player->is_rankable = 0;
	player->is_modified = IS_MODIFIED_CREATED;

	return 1;
}

static int skip_elo(FILE *file, const char *path)
{
	fscanf(file, " %*d");
	return 1;
}

static int skip_rank(FILE *file, const char *path)
{
	fscanf(file, " %*u");
	return 1;
}

static int read_elo(FILE *file, const char *path, void *buf)
{
	int ret;

	errno = 0;
	ret = fscanf(file, " %d", (int*)buf);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match elo\n", path), 0;

	return 1;
}

static int read_rank(FILE *file, const char *path, void *buf)
{
	int ret;

	errno = 0;
	ret = fscanf(file, " %u", (unsigned*)buf);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match rank\n", path), 0;

	return 1;
}

static int read_clan(char *clan, FILE *file, const char *path)
{
	int ret;

	errno = 0;
	ret = fscanf(file, " %s\n", clan);

	if (ret == EOF && errno != 0) {
		perror(path);
		return 0;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match player clan\n", path);
		return 0;
	}

	return 1;
}

int read_player(struct player *player, char *name)
{
	char *path;
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
	reset_player(player, name);

	if (!(path = get_path(name)))
		return 0;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return new_player(player);
		else
			return perror(path), 0;
	}

	if (!read_clan(player->clan, file, path))
		goto fail;

	if (!read_historic(&player->elo_historic, file, path, read_elo))
		goto fail;

	if (!read_historic(&player->rank_historic, file, path, read_rank))
		goto fail;

	if (&player->elo_historic.nrecords > 0)
		player->elo = *(int*)record_data(&player->elo_historic, last_record(&player->elo_historic));
	if (&player->rank_historic.nrecords > 0)
		player->rank = *(unsigned*)record_data(&player->rank_historic, last_record(&player->rank_historic));

	fclose(file);
	return 1;

fail:
	fclose(file);
	return 0;
}

static int write_elo(FILE *file, const char *path, void *buf)
{
	if (fprintf(file, "%d", *(int*)buf) < 0)
		return perror(path), 0;
	return 1;
}

static int write_rank(FILE *file, const char *path, void *buf)
{
	if (fprintf(file, "%d", *(unsigned*)buf) < 0)
		return perror(path), 0;
	return 1;
}

static int write_clan(const char *clan, FILE *file, const char *path)
{
	if (fprintf(file, "%s\n", clan) < 0)
		return perror(path), 0;

	return 1;
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

	if (!write_clan(player->clan, file, path))
		goto fail;

	if (!write_historic(&player->elo_historic, file, path, write_elo))
		goto fail;
	if (!write_historic(&player->rank_historic, file, path, write_rank))
		goto fail;

	fclose(file);
	return 1;
fail:
	fclose(file);
	return 0;
}

/*
 * Update player ELO points, update history if necessary.
 */
int set_elo(struct player *player, int elo)
{
	assert(player != NULL);

	append_record(&player->elo_historic, &elo);
	player->elo = elo;
	player->is_modified |= IS_MODIFIED_ELO;

	return 1;
}

int set_rank(struct player *player, unsigned rank)
{
	assert(player != NULL);

	append_record(&player->rank_historic, &rank);
	player->rank = rank;
	player->is_modified |= IS_MODIFIED_RANK;

	return 1;
}

void set_clan(struct player *player, char *clan)
{
	assert(player != NULL);
	assert(clan != NULL);
	assert(is_valid_hexname(clan));

	strcpy(player->clan, clan);
	player->is_modified |= IS_MODIFIED_CLAN;
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

static void init_player_summary(struct player_summary *ps)
{
	strcpy(ps->clan, "00");
	ps->elo = INVALID_ELO;
	ps->rank = INVALID_RANK;
}

int read_player_summary(struct player_summary *ps, char *name)
{
	char *path;
	FILE *file;

	init_player_summary(ps);
	strcpy(ps->name, name);

	if (!(path = get_path(name)))
		return 0;

	if (!(file = fopen(path, "r")))
		return perror(path), 0;

	if (!read_clan(ps->clan, file, path))
		goto fail;

	if (!read_historic_summary(&ps->elo_hs, file, path, read_elo, skip_elo, &ps->elo))
		goto fail;

	if (!read_historic_summary(&ps->rank_hs, file, path, read_rank, skip_rank, &ps->rank))
		goto fail;

	fclose(file);
	return 1;

fail:
	fclose(file);
	return 0;
}
