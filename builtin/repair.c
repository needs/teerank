#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "player.h"
#include "clan.h"

struct clan_list {
	unsigned length;
	struct clan *clans;
};

static struct clan *new_clan(struct clan_list *list, char *name)
{
	const unsigned OFFSET = 1024;
	struct clan *clan;

	assert(list != NULL);
	assert(name != NULL);
	assert(strcmp(name, "00"));

	if (list->length % OFFSET == 0) {
		struct clan *clans;
		clans = realloc(list->clans,
		                (list->length + OFFSET) * sizeof(*clans));
		if (!clans)
			return perror("realloc(clans)"), NULL;
		list->clans = clans;
	}

	clan = &list->clans[list->length++];
	create_clan(clan, name, 0);

	return clan;
}

static struct clan *get_clan(struct clan_list *list, char *name)
{
	unsigned i;

	if (!strcmp(name, "00"))
		return NULL;

	for (i = 0; i < list->length; i++)
		if (!strcmp(list->clans[i].name, name))
			return &list->clans[i];

	return new_clan(list, name);
}

static const struct clan CLAN_ZERO;

/*
 * A clan need a repair when the entry on the disk is different than
 * the given one.
 */
static int need_repair(struct clan *clan)
{
	struct clan tmp = CLAN_ZERO;

	assert(clan != NULL);

	if (!read_clan(&tmp, clan->name))
		return 1;
	return !clan_equal(&tmp, clan);
}

static const struct clan_list CLAN_LIST_ZERO;

int main(int argc, char **argv)
{
	DIR *dir;
	struct dirent *dp;
	struct clan_list clans = CLAN_LIST_ZERO;
	char path[PATH_MAX];
	unsigned i;
	unsigned nrepair = 0;
	struct player player;

	load_config(1);

	if (!dbpath(path, PATH_MAX, "players/"))
		return EXIT_FAILURE;

	if (!(dir = opendir(path)))
		return perror(path), EXIT_FAILURE;

	/* Build clan list */
	init_player(&player);
	while ((dp = readdir(dir))) {
		struct clan *clan;

		if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
			continue;
		if (!is_valid_hexname(dp->d_name))
			continue;

		if (read_player(&player, dp->d_name) != SUCCESS)
			continue;

		clan = get_clan(&clans, player.clan);
		if (clan)
			add_member(clan, player.name);
	}

	closedir(dir);

	/* Update clan that are not up-to-date */
	for (i = 0; i < clans.length; i++) {
		if (need_repair(&clans.clans[i])) {
			write_clan(&clans.clans[i]);
			verbose("Repaired %s\n", clans.clans[i].name);
			nrepair++;
		}
	}

	verbose("%u clans repaired\n", nrepair);

	return EXIT_SUCCESS;
}
