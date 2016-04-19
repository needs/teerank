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

static void init_player(struct player *player, char *name)
{
	strcpy(player->name, name);
	strcpy(player->clan, "00");
	player->rank = INVALID_RANK;
	player->elo  = INVALID_ELO;
	player->is_modified = 0;
}

static int create_player(struct player *player, char *name)
{
	assert(name != NULL);
	assert(player != NULL);

	init_player(player, name);
	player->elo = DEFAULT_ELO;
	player->is_rankable = 0;
	player->is_modified = IS_MODIFIED_CREATED;

	return 1;
}

int read_player(struct player *player, char *name)
{
	char *path;
	int ret;
	FILE *file;

	assert(name != NULL);
	assert(player != NULL);
	assert(is_valid_hexname(name));

	if (!(path = get_path(name)))
		return 0;
	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return create_player(player, name);
		else
			return perror(path), 0;
	}

	init_player(player, name);

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
		fprintf(stderr, "%s: Cannot match player rank\n", path);
		goto fail;
	} else if (ret == 2) {
		fprintf(stderr, "%s: Cannot match player score\n", path);
		goto fail;
	}

	fclose(file);
	return 1;

fail:
	fclose(file);
	return 0;
}

int write_player(struct player *player)
{
	char *path;
	FILE *file;
	int ret;

	assert(player != NULL);
	assert(player->name[0] != '\0');

	if (!(path = get_path(player->name)))
		return 0;
	if (!(file = fopen(path, "w")))
		return perror(path), 0;

	ret = fprintf(file, "%s %d %u\n", player->clan,
	              player->elo, player->rank);
	if (ret < 0) {
		perror(path);
		fclose(file);
		return 0;
	}

	fclose(file);
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
