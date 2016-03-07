#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utime.h>

#include "io.h"
#include "config.h"
#include "player.h"

struct clan {
	char name[MAX_NAME_LENGTH];

	unsigned length;
	struct player *players;
};

struct clan_array {
	unsigned length;
	struct clan *clans;
};

static struct clan *add_clan(struct clan_array *array, struct clan *clan)
{
	static const unsigned OFFSET = 1024;

	if (array->length % OFFSET == 0) {
		struct clan *clans;
		clans = realloc(array->clans, (array->length + OFFSET) * sizeof(*array->clans));
		if (!clans)
			return perror("realloc(clans)"), NULL;
		array->clans = clans;
	}

	array->clans[array->length] = *clan;
	return &array->clans[array->length++];
}

static struct player *add_member(struct clan *clan, struct player *player)
{
	static const unsigned OFFSET = 1024;

	assert(clan != NULL);
	assert(player != NULL);
	assert(strcmp(clan->name, "00"));

	if (clan->length % OFFSET == 0) {
		struct player *players;

		players = realloc(clan->players, (clan->length + OFFSET) * sizeof(*clan->players));
		if (!players)
			return perror("realloc(players)"), NULL;
		clan->players = players;
	}

	clan->players[clan->length] = *player;
	return &clan->players[clan->length++];
}

static struct clan CLAN_ZERO;

static struct clan *get_clan(struct clan_array *array, struct player *player)
{
	unsigned i;
	struct clan clan = CLAN_ZERO;

	assert(array != NULL);
	assert(player != NULL);

	if (!strcmp(player->clan, "00"))
		return NULL;

	/* Search for clan, and if not found, add it */
	for (i = 0; i < array->length; i++)
		if (!strcmp(array->clans[i].name, player->clan))
			return &array->clans[i];

	strcpy(clan.name, player->clan);
	return add_clan(array, &clan);
}

static void write_clan_array(struct clan_array *clans)
{
	unsigned i, j;

	assert(clans != NULL);

	for (i = 0; i < clans->length; i++) {
		struct clan *clan = &clans->clans[i];
		char path[PATH_MAX];
		FILE *file;

		sprintf(path, "%s/clans/%s", config.root, clan->name);
		if (!(file = fopen(path, "w"))) {
			perror(path);
			continue;
		}
		for (j = 0; j < clan->length; j++)
			fprintf(file, "%s\n", clan->players[j].name);
		fclose(file);
	}
}

static const struct clan_array CLAN_ARRAY_ZERO;

int main(int argc, char **argv)
{
	DIR *dir;
	struct dirent *dp;
	struct clan_array clans = CLAN_ARRAY_ZERO;
	char path[PATH_MAX];

	load_config();
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Build in memory the list of all clans with their members */
	sprintf(path, "%s/players", config.root);
	if (!(dir = opendir(path)))
		return perror(path), EXIT_FAILURE;
	while ((dp = readdir(dir))) {
		struct player player;
		struct clan *clan;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (!read_player(&player, dp->d_name))
			continue;
		if (!(clan = get_clan(&clans, &player)))
			continue;
		if (!(add_member(clan, &player)))
			continue;
	}
	closedir(dir);

	write_clan_array(&clans);

	return EXIT_SUCCESS;
}
