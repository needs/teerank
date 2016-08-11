#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "player.h"
#include "config.h"
#include "elo.h"

static int is_valid_hexpair(const char *hex)
{
	char c1, c2;

	if (hex[0] == '\0' || hex[1] == '\0')
		return 0;

	c1 = hex[0];
	c2 = hex[1];

	if (!isxdigit(c1) || !isxdigit(c2))
		return 0;

	if (c1 == '0' && c2 == '0')
		return 0;

	return 1;
}

static int is_terminating_hexpair(const char *hex)
{
	if (hex[0] == '\0' || hex[1] == '\0')
		return 0;

	return hex[0] == '0' && hex[1] == '0' && hex[2] == '\0';
}

int is_valid_hexname(const char *hex)
{
	assert(hex != NULL);

	while (is_valid_hexpair(hex))
		hex += 2;

	return is_terminating_hexpair(hex);
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

void init_player(struct player *player)
{
	static const struct player PLAYER_ZERO;
	*player = PLAYER_ZERO;

	init_historic(&player->hist, sizeof(struct player_record),  UINT_MAX);
}

/*
 * Set all player's fields to meaningless values suitable for printing.
 */
static void reset_player(struct player *player, const char *name)
{
	strcpy(player->name, name);
	strcpy(player->clan, "00");
	player->elo = INVALID_ELO;
	player->rank = UNRANKED;

	player->is_modified = 0;
	player->delta = NULL;
}

static void reset_player_info(struct player_info *player, const char *name)
{
	strcpy(player->name, name);
	strcpy(player->clan, "00");
	player->elo = INVALID_ELO;
	player->rank = UNRANKED;
}

void create_player(struct player *player, const char *name)
{
	time_t now;

	assert(player != NULL);

	strcpy(player->name, name);
	strcpy(player->clan, "00");

	create_historic(&player->hist);

	set_elo(player, DEFAULT_ELO);
	set_rank(player, UNRANKED);

	now = time(NULL);
	player->last_seen = *gmtime(&now);

	player->is_rankable = 0;
	player->is_modified = IS_MODIFIED_CREATED;
}

static int read_player_record(FILE *file, const char *path, void *buf)
{
	int ret;
	struct player_record *rec = buf;

	errno = 0;
	ret = fscanf(file, " %d %u", &rec->elo, &rec->rank);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match elo\n", path), 0;
	else if (ret == 1)
		return fprintf(stderr, "%s: Cannot match rank\n", path), 0;

	return 1;
}

static int read_clan(FILE *file, const char *path, char *clan)
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

static int read_last_seen(FILE *file, const char *path, struct tm *last_seen)
{
	/* RFC-3339 */
	char buf[] = "yyyy-mm-ddThh-mm-ssZ";

	if (fread(buf, sizeof(buf) - 1, 1, file) != 1) {
		if (ferror(file)) {
			perror(path);
			return 0;
		} else {
			fprintf(stderr, "%s: Early end of file\n", path);
			return 0;
		}
	}

	if (strptime(buf, "%Y-%m-%dT%H:%M:%SZ", last_seen) == NULL) {
		fprintf(stderr, "%s: Cannot match player last seen date\n", path);
		return 0;
	}

	return 1;
}

static int read_player_header(FILE *file, const char *path, struct player *player)
{
	struct player_record rec;

	if (!read_last_seen(file, path, &player->last_seen))
		return 0;
	if (!read_clan(file, path, player->clan))
		return 0;
	if (!read_player_record(file, path, &rec))
		return 0;

	player->elo = rec.elo;
	player->rank = rec.rank;

	return 1;
}

enum read_player_ret read_player(struct player *player, const char *name)
{
	FILE *file = NULL;
	char path[PATH_MAX];

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

	if (!dbpath(path, PATH_MAX, "players/%s", name))
		goto fail;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return PLAYER_NOT_FOUND;

		perror(path);
		goto fail;
	}

	if (!read_player_header(file, path, player))
		goto fail;
	if (!read_historic(&player->hist, file, path, read_player_record))
		goto fail;

	/* Historics cannot be empty */
	assert(player->hist.nrecords > 0);

	fclose(file);
	return PLAYER_FOUND;

fail:
	if (file)
		fclose(file);
	return PLAYER_ERROR;
}

static int write_player_record(FILE *file, const char *path, void *buf)
{
	struct player_record *rec = buf;

	if (fprintf(file, "%d %u", rec->elo, rec->rank) < 0) {
		perror(path);
		return 0;
	}

	return 1;
}

static int write_clan(FILE *file, const char *path, const char *clan)
{
	if (fprintf(file, "%s\n", clan) < 0) {
		perror(path);
		return 0;
	}

	return 1;
}

static int write_last_seen(FILE *file, const char *path, struct tm *last_seen)
{
	/* RFC-3339 */
	char buf[] = "yyyy-mm-ddThh-mm-ssZ";
	size_t ret;

	ret = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", last_seen);
	if (ret != sizeof(buf) - 1) {
		fprintf(stderr, "%s: Cannot represent date using RFC-3339\n", path);
		return 0;
	}

	if (fprintf(file, "%s\n", buf) < 0) {
		perror(path);
		return 0;
	}

	return 1;
}

static int write_player_header(FILE *file, const char *path, struct player *player)
{
	struct player_record rec;

	rec.elo = player->elo;
	rec.rank = player->rank;

	if (!write_last_seen(file, path, &player->last_seen))
		return 0;
	if (!write_clan(file, path, player->clan))
		return 0;
	if (!write_player_record(file, path, &rec))
		return 0;
	fputc('\n', file);

	return 1;
}

int write_player(struct player *player)
{
	FILE *file = NULL;
	char path[PATH_MAX];

	assert(player != NULL);
	assert(player->name[0] != '\0');

	if (!dbpath(path, PATH_MAX, "players/%s", player->name))
		goto fail;

	if (!(file = fopen(path, "w"))) {
		perror(path);
		goto fail;
	}

	if (!write_player_header(file, path, player))
		goto fail;
	if (!write_historic(&player->hist, file, path, write_player_record))
		goto fail;

	fclose(file);
	return 1;
fail:
	if (file)
		fclose(file);
	return 0;
}

void set_elo(struct player *player, int elo)
{
	struct player_record *last = NULL;

	assert(player != NULL);

	player->elo = elo;
	player->is_modified |= IS_MODIFIED_ELO;

	if (player->hist.last)
		last = record_data(&player->hist, player->hist.last);

	if (last && last->rank == UNRANKED) {
		last->elo = elo;
	} else {
		struct player_record rec;

		rec.elo = elo;
		rec.rank = UNRANKED;

		append_record(&player->hist, &rec);
	}
}

void set_rank(struct player *player, unsigned rank)
{
	struct player_record *rec;

	assert(player != NULL);

	player->rank = rank;
	player->is_modified |= IS_MODIFIED_RANK;

	rec = record_data(&player->hist, player->hist.last);
	if (rec->rank == UNRANKED)
		rec->rank = rank;
}

void set_clan(struct player *player, char *clan)
{
	assert(player != NULL);
	assert(clan != NULL);
	assert(is_valid_hexname(clan));

	strcpy(player->clan, clan);
	player->is_modified |= IS_MODIFIED_CLAN;
}

static void init_player_info(struct player_info *ps)
{
	strcpy(ps->clan, "00");
	ps->elo = INVALID_ELO;
	ps->rank = UNRANKED;
}

static int read_player_info_header(
	FILE *file, const char *path, struct player_info *ps)
{
	struct player_record rec;

	if (!read_last_seen(file, path, &ps->last_seen))
		return 0;
	if (!read_clan(file, path, ps->clan))
		return 0;
	if (!read_player_record(file, path, &rec))
		return 0;

	ps->elo = rec.elo;
	ps->rank = rec.rank;

	return 1;
}

enum read_player_ret read_player_info(struct player_info *ps, const char *name)
{
	FILE *file = NULL;
	char path[PATH_MAX];

	init_player_info(ps);
	strcpy(ps->name, name);

	reset_player_info(ps, name);

	if (!dbpath(path, PATH_MAX, "players/%s", name))
		goto fail;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return PLAYER_NOT_FOUND;
		perror(path);
		goto fail;
	}

	if (!read_player_info_header(file, path, ps))
		goto fail;
	if (!read_historic_info(&ps->hist, file, path))
		goto fail;

	fclose(file);
	return PLAYER_FOUND;

fail:
	if (file)
		fclose(file);
	return PLAYER_ERROR;
}
