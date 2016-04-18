#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "clan.h"

struct player *add_member(struct clan *clan, char *name)
{
	struct player *member;
	const unsigned OFFSET = 1024;

	assert(clan != NULL);
	assert(name != NULL);

	if (clan->length % OFFSET == 0) {
		struct player *members;
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

int read_clan(struct clan *clan, char *cname)
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

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT) {
			clan->length = 0;
			clan->members = NULL;
			strcpy(clan->name, cname);
			return 1;
		} else {
			perror(path);
			return 0;
		}
	}

	/* Ignore failure for now... */
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

int write_clan(const struct clan *clan)
{
	unsigned i;
	static char path[PATH_MAX];
	int ret;
	FILE *file;

	assert(clan != NULL);
	assert(strcmp(clan->name, "00"));

	ret = snprintf(path, PATH_MAX, "%s/clans/%s", config.root, clan->name);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(dest, %d): Too long\n", PATH_MAX);
		return 0;
	}

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

struct player *get_member(struct clan *clan, char *name)
{
	unsigned i;

	for (i = 0; i < clan->length; i++)
		if (!strcmp(clan->members[i].name, name))
			return &clan->members[i];

	return NULL;
}

void remove_member(struct clan *clan, struct player *member)
{
	struct player *last;

	assert(clan != NULL);
	assert(member - clan->members < clan->length);

	last = &clan->members[clan->length - 1];

	*member = *last;
	clan->length--;
}

unsigned load_members(struct clan *clan)
{
	unsigned i;
	unsigned count = 0;

	for (i = 0; i < clan->length; i++) {
		if (!read_player(&clan->members[i], clan->members[i].name)) {
			remove_member(clan, &clan->members[i]);
			count++;
		}
	}

	return count;
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
