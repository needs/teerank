#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>

#include "io.h"

/* Make path handling easier */
static char path[PATH_MAX];

/* Defined by argv[1] and argv[2] */
static char *clans_directory, *players_directory;

struct player;
struct clan {
	char name[MAX_NAME_LENGTH];

	unsigned length;
	struct player *players;
};

struct clan_array {
	unsigned length;
	struct clan *clans;
};

struct player {
	char name[MAX_NAME_LENGTH];
	char clan[MAX_NAME_LENGTH];
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

static struct player *add_player(struct clan *clan, struct player *player)
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

static int init_player(struct player *player, char *name)
{
	FILE *file;

	assert(player != NULL);

	strcpy(player->name, name);

	/* No clan file means empty clan name */
	sprintf(path, "%s/%s/%s", players_directory, name, "clan");
	if ((file = fopen(path, "r"))) {
		if (!fgets(player->clan, MAX_NAME_LENGTH, file))
			return perror(path), fclose(file), 0;
		fclose(file);
	} else if (errno != ENOENT) {
		return perror(path), 0;
	} else {
		strcpy(player->clan, "00");
	}

	return 1;
}

static void write_clan_array(struct clan_array *clans)
{
	unsigned i, j;

	assert(clans != NULL);

	for (i = 0; i < clans->length; i++) {
		struct clan *clan = &clans->clans[i];
		FILE *file;

		/* Create directory */
		sprintf(path, "%s/%s", clans_directory, clan->name);
		if (mkdir(path, S_IRWXU) == -1 && errno != EEXIST) {
			perror(path);
			continue;
		}

		/* Write member list */
		sprintf(path, "%s/%s/%s", clans_directory, clan->name, "members");
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

	if (argc != 3) {
		fprintf(stderr, "usage: %s <clans_directory> <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	clans_directory = argv[1];
	players_directory = argv[2];

	/* Build in memory the list of all clans with their members */
	dir = opendir(players_directory);
	while ((dp = readdir(dir))) {
		struct player player;
		struct clan *clan;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (!init_player(&player, dp->d_name))
			continue;
		if (!(clan = get_clan(&clans, &player)))
			continue;
		if (!(add_player(clan, &player)))
			continue;
	}
	closedir(dir);

	write_clan_array(&clans);

	return EXIT_SUCCESS;
}
