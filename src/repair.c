#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "player.h"

struct member {
	char name[MAX_NAME_HEX_LENGTH];
};

struct clan {
	char name[MAX_CLAN_HEX_LENGTH];

	unsigned length;
	struct member *members;
};

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
	strcpy(clan->name, name);
	clan->length = 0;
	clan->members = NULL;

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

static void add_member(struct clan *clan, char *name)
{
	struct member *member;
	const unsigned OFFSET = 1024;

	assert(clan != NULL);
	assert(name != NULL);

	if (clan->length % OFFSET == 0) {
		struct member *members;
		members = realloc(clan->members,
		                  (clan->length + OFFSET) * sizeof(*members));
		if (!members)
			return perror("realloc(members)");
		clan->members = members;
	}

	member = &clan->members[clan->length++];
	strcpy(member->name, name);
}

static void write_clan(struct clan *clan)
{
	unsigned i;
	static char path[PATH_MAX], dest[PATH_MAX];
	int fd, ret;
	FILE *file;

	assert(clan != NULL);
	assert(strcmp(clan->name, "00"));

	ret = snprintf(path, PATH_MAX, "%s/clans/%s-tmp-XXXXXX",
	               config.root, clan->name);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(path, %d): Too long\n", PATH_MAX);
		return;
	}

	ret = snprintf(dest, PATH_MAX, "%s/clans/%s", config.root, clan->name);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(dest, %d): Too long\n", PATH_MAX);
		return;
	}

	if ((fd = mkstemp(path)) == -1) {
		fprintf(stderr, "mkstemp(\"%s\"): %s\n", path, strerror(errno));
		return;
	}
	if (!(file = fdopen(fd, "w"))) {
		perror("fdopen()");
		close(fd); unlink(path);
		return;
	}

	for (i = 0; i < clan->length; i++)
		fprintf(file, "%s\n", clan->members[i].name);

	fclose(file);

	if (rename(path, dest) == -1) {
		fprintf(stderr, "rename(\"%s\", \"%s\"): %s\n",
		        path, dest, strerror(errno));
		unlink(path);
		return;
	}
}

static int read_clan(struct clan *clan, char *cname)
{
	static char path[PATH_MAX];
	char pname[MAX_NAME_HEX_LENGTH];
	int ret;
	FILE *file;

	assert(clan != NULL);
	assert(strcmp(cname, "00"));

	ret = snprintf(path, PATH_MAX, "%s/clans/%s", config.root, cname);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(path, %d): Too long\n", PATH_MAX);
		return 0;
	}

	if (!(file = fopen(path, "r")))
		return perror(path), 0;

	while (fscanf(file, " %s", pname) == 1)
		add_member(clan, pname);

	if (ferror(file)) {
		perror(path);
		goto fail;
	} else if (ret == 0) {
		fprintf(stderr, "%s: Cannot match player name\n", path);
		goto fail;
	}

	strcpy(clan->name, cname);
	fclose(file);
	return 1;

fail:
	fclose(file);
	return 0;
}

/*
 * Compare two clans, return 1 if they are exactly the same.
 */
static int clan_equal(struct clan *c1, struct clan *c2)
{
	unsigned i, j;

	assert(c1 != NULL);
	assert(c2 != NULL);

	if (c1->length != c2->length)
		return 0;
	if (strcmp(c1->name, c2->name))
		return 0;

	for (i = 0; i < c1->length; i++) {
		for (j = 0; j < c2->length; j++) {
			if (!strcmp(c1->members[i].name, c2->members[j].name))
				break;
		}

		if (j == c2->length)
			return 0;
	}

	return 1;
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
	static char path[PATH_MAX];
	unsigned i;
	unsigned nrepair = 0;

	load_config();

	if (snprintf(path, PATH_MAX, "%s/players/", config.root) >= PATH_MAX) {
		fprintf(stderr, "snprintf(path, %d): Too long\n", PATH_MAX);
		return EXIT_FAILURE;
	}

	if (!(dir = opendir(path)))
		return perror(path), EXIT_FAILURE;

	/* Build clan list */
	while ((dp = readdir(dir))) {
		struct player player;
		struct clan *clan;

		if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
			continue;
		if (!read_player(&player, dp->d_name))
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
