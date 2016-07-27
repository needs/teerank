#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "clan.h"
#include "index.h"

struct player_info *add_member(struct clan *clan, char *name)
{
	struct player_info *member;
	const unsigned OFFSET = 1024;

	assert(clan != NULL);
	assert(name != NULL);

	if (!is_valid_hexname(name)) {
		fprintf(stderr, "%s: Not a valid hexname\n", name);
		return NULL;
	}

	if (clan->length % OFFSET == 0) {
		struct player_info *members;
		members = realloc(clan->members,
		                  (clan->length + OFFSET) * sizeof(*members));
		if (!members)
			return perror("realloc(members)"), NULL;
		clan->members = members;
	}

	member = &clan->members[clan->length++];
	strcpy(member->name, name);
	return member;
}

void create_clan(struct clan *clan, const char *name)
{
	clan->length = 0;
	clan->members = NULL;
	strcpy(clan->name, name);
}

static const struct clan CLAN_ZERO;

int read_clan(struct clan *clan, const char *cname)
{
	FILE *file = NULL;
	char path[PATH_MAX];

	char pname[HEXNAME_LENGTH];
	int ret;

	assert(clan != NULL);
	assert(is_valid_hexname(cname));
	assert(strcmp(cname, "00"));

	if (!dbpath(path, PATH_MAX, "clans/%s", cname))
		goto fail;

	*clan = CLAN_ZERO;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			goto not_found;
		perror(path);
		goto fail;
	}

	/* Ignore failure for now... */
	while ((ret = fscanf(file, " %s", pname)) == 1)
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
	return CLAN_FOUND;

fail:
	if (file)
		fclose(file);
	return CLAN_ERROR;
not_found:
	if (file)
		fclose(file);
	return CLAN_NOT_FOUND;
}

int write_clan(const struct clan *clan)
{
	FILE *file;
	char path[PATH_MAX];

	unsigned i;

	assert(clan != NULL);
	assert(strcmp(clan->name, "00"));

	if (!dbpath(path, PATH_MAX, "clans/%s", clan->name))
		return 0;

	if (!(file = fopen(path, "w"))) {
		perror(path);
		return 0;
	}

	for (i = 0; i < clan->length; i++)
		fprintf(file, "%s\n", clan->members[i].name);

	fclose(file);

	return 1;
}

void free_clan(struct clan *clan)
{
	if (clan->members)
		free(clan->members);

	clan->length = 0;
	clan->members = NULL;
}

struct player_info *get_member(struct clan *clan, char *name)
{
	unsigned i;

	for (i = 0; i < clan->length; i++)
		if (!strcmp(clan->members[i].name, name))
			return &clan->members[i];

	return NULL;
}

void remove_member(struct clan *clan, struct player_info *member)
{
	struct player_info *last;

	assert(clan != NULL);
	assert(member - clan->members < clan->length);

	last = &clan->members[clan->length - 1];

	*member = *last;
	clan->length--;
}

unsigned load_members(struct clan *clan)
{
	unsigned i;
	unsigned removed = 0;
	enum read_player_ret ret;

	for (i = 0; i < clan->length; i++) {
		ret = read_player_info(&clan->members[i], clan->members[i].name);

		if (ret != PLAYER_FOUND) {
			remove_member(clan, &clan->members[i]);
			removed++;
		}
	}

	return removed;
}

int clan_equal(const struct clan *c1, const struct clan *c2)
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

int add_member_inline(char *clan, char *player)
{
	FILE *file;
	char path[PATH_MAX];

	assert(clan != NULL);
	assert(player != NULL);
	assert(strcmp(clan, "00") != 0);

	if (!dbpath(path, PATH_MAX, "clans/%s", clan))
		return 0;

	if (!(file = fopen(path, "a")))
		return perror(path), 0;
	fprintf(file, "%s\n", player);
	fclose(file);

	return 1;
}
