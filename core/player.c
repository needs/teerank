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
	player->lastseen = *gmtime(&NEVER_SEEN);

	player->clan_changed = 0;
	player->delta = NULL;
}

static void reset_player_info(struct player_info *player, const char *name)
{
	strcpy(player->name, name);
	strcpy(player->clan, "00");
	player->elo = INVALID_ELO;
	player->rank = UNRANKED;
	player->lastseen = *gmtime(&NEVER_SEEN);
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
	player->lastseen = *gmtime(&now);

	player->is_rankable = 0;
}

static int read_player_record(struct jfile *jfile, void *buf)
{
	struct player_record *rec = buf;

	json_read_int(jfile, NULL, &rec->elo);
	json_read_unsigned(jfile, NULL, &rec->rank);

	return !json_have_error(jfile);
}

static void read_player_header(struct jfile *jfile, struct player *player)
{
	json_read_string(  jfile, "name"    , player->name, sizeof(player->name));
	json_read_string(  jfile, "clan"    , player->clan, sizeof(player->clan));
	json_read_int(     jfile, "elo"     , &player->elo);
	json_read_unsigned(jfile, "rank"    , &player->rank);
	json_read_tm(      jfile, "lastseen", &player->lastseen);
}

int read_player(struct player *player, const char *name)
{
	FILE *file = NULL;
	struct jfile jfile;
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
			return NOT_FOUND;

		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	json_read_object_start(&jfile, NULL);
	read_player_header(&jfile, player);

	if (json_have_error(&jfile))
		goto fail;

	if (!read_historic(&player->hist, &jfile, read_player_record))
		goto fail;

	if (!json_read_object_end(&jfile))
		goto fail;

	fclose(file);

	/* Historics cannot be empty */
	assert(player->hist.nrecords > 0);

	return SUCCESS;

fail:
	if (file)
		fclose(file);
	return FAILURE;
}

static int write_player_record(struct jfile *jfile, void *buf)
{
	struct player_record *rec = buf;

	json_write_int(     jfile, NULL, rec->elo);
	json_write_unsigned(jfile, NULL, rec->rank);

	return !json_have_error(jfile);
}

static void write_player_header(struct jfile *jfile, struct player *player)
{
	json_write_string(  jfile, "name"    , player->name, sizeof(player->name));
	json_write_string(  jfile, "clan"    , player->clan, sizeof(player->clan));
	json_write_int(     jfile, "elo"     , player->elo);
	json_write_unsigned(jfile, "rank"    , player->rank);
	json_write_tm(      jfile, "lastseen", player->lastseen);
}

int write_player(struct player *player)
{
	struct jfile jfile;
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

	json_init(&jfile, file, path);

	json_write_object_start(&jfile, NULL);
	write_player_header(&jfile, player);

	if (json_have_error(&jfile))
		goto fail;

	if (!write_historic(&player->hist, &jfile, write_player_record))
		goto fail;

	if (!json_write_object_end(&jfile))
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
	player->clan_changed = 1;
}

void set_lastseen(struct player *player)
{
	time_t now = time(NULL);

	assert(player != NULL);

	player->lastseen = *gmtime(&now);
}

static void init_player_info(struct player_info *ps)
{
	strcpy(ps->clan, "00");
	ps->elo = INVALID_ELO;
	ps->rank = UNRANKED;
	ps->lastseen = (struct tm){ 0 };
}

static void read_player_info_header(
	struct jfile *jfile, struct player_info *ps)
{
	json_read_string(  jfile, "name", ps->name, sizeof(ps->name));
	json_read_string(  jfile, "clan", ps->clan, sizeof(ps->clan));
	json_read_int(     jfile, "elo", &ps->elo);
	json_read_unsigned(jfile, "rank", &ps->rank);
	json_read_tm(      jfile, "lastseen", &ps->lastseen);
}

int read_player_info(struct player_info *ps, const char *name)
{
	struct jfile jfile;
	FILE *file = NULL;
	char path[PATH_MAX];

	init_player_info(ps);
	reset_player_info(ps, name);

	if (!dbpath(path, PATH_MAX, "players/%s", name))
		goto fail;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return NOT_FOUND;
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	json_read_object_start(&jfile, NULL);
	read_player_info_header(&jfile, ps);
	read_historic_info(&ps->hist, &jfile);

	if (json_have_error(&jfile))
		goto fail;

	fclose(file);
	return SUCCESS;

fail:
	if (file)
		fclose(file);
	return FAILURE;
}
