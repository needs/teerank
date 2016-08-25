#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "clan.h"
#include "index.h"
#include "json.h"

/*
 * Does grow the internal array of member if needed, but does not update
 * clan->nmembers.
 */
static int alloc_clan(struct clan *clan, size_t nmembers)
{
	struct player_info *members;
	const unsigned ALLOC_STEP = 1024;
	unsigned max_nmembers;

	max_nmembers = nmembers - (nmembers % ALLOC_STEP) + ALLOC_STEP;

	/* Is the array is already wide enough? */
	if (max_nmembers <= clan->max_nmembers)
		return 1;

	members = realloc(clan->members, max_nmembers * sizeof(*members));
	if (!members)
		return perror("realloc(members)"), 0;

	clan->members = members;
	clan->max_nmembers = max_nmembers;

	return 1;
}

struct player_info *add_member(struct clan *clan, char *name)
{
	struct player_info *member;

	assert(clan != NULL);
	assert(name != NULL);

	if (!is_valid_hexname(name)) {
		fprintf(stderr, "%s: Not a valid hexname\n", name);
		return NULL;
	}

	if (!alloc_clan(clan, clan->nmembers + 1))
		return NULL;

	member = &clan->members[clan->nmembers++];
	strcpy(member->name, name);
	return member;
}

static const struct clan CLAN_ZERO;

void create_clan(struct clan *clan, const char *name, unsigned nmembers)
{
	*clan = CLAN_ZERO;

	alloc_clan(clan, nmembers);

	strcpy(clan->name, name);
	clan->nmembers = 0;
}

int read_clan(struct clan *clan, const char *cname)
{
	FILE *file = NULL;
	struct jfile jfile;
	char path[PATH_MAX];
	unsigned i, nmembers;

	assert(clan != NULL);
	assert(is_valid_hexname(cname));
	assert(strcmp(cname, "00") != 0);

	*clan = CLAN_ZERO;

	if (!dbpath(path, PATH_MAX, "clans/%s", cname))
		goto fail;

	if ((file = fopen(path, "r")) == NULL) {
		if (errno == ENOENT) {
			create_clan(clan, cname, 0);
			return CLAN_NOT_FOUND;
		}
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	if (!json_read_object_start(&jfile, NULL))
		goto fail;

	if (!json_read_unsigned(&jfile, "nmembers", &nmembers))
		goto fail;

	/*
	 * Add member also clann alloc_clan(), but since we already know
	 * the final array size, we can just resize the array once for
	 * all and make the whole process faster.
	 */
	if (!alloc_clan(clan, nmembers))
		goto fail;

	if (!json_read_array_start(&jfile, "members"))
		goto fail;

	for (i = 0; i < nmembers; i++) {
		char pname[HEXNAME_LENGTH];

		if (!json_read_string(&jfile, NULL, pname, sizeof(pname)))
			goto fail;

		add_member(clan, pname);
	}

	if (!json_read_array_end(&jfile))
		goto fail;

	if (!json_read_object_end(&jfile))
		goto fail;

	fclose(file);

	strcpy(clan->name, cname);

	return CLAN_FOUND;

fail:
	if (file)
		fclose(file);
	return CLAN_ERROR;
}

int write_clan(const struct clan *clan)
{
	FILE *file = NULL;
	struct jfile jfile;
	char path[PATH_MAX];

	unsigned i;

	assert(clan != NULL);
	assert(strcmp(clan->name, "00"));

	if (!dbpath(path, PATH_MAX, "clans/%s", clan->name))
		goto fail;

	if ((file = fopen(path, "w")) == NULL) {
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	if (!json_write_object_start(&jfile, NULL))
		goto fail;

	if (!json_write_unsigned(&jfile, "nmembers", clan->nmembers))
		goto fail;

	if (!json_write_array_start(&jfile, "members"))
		goto fail;

	for (i = 0; i < clan->nmembers; i++)
		if (!json_write_string(
			    &jfile, NULL, clan->members[i].name,
			    sizeof(clan->members[i].name)))
			goto fail;

	if (!json_write_array_end(&jfile))
		goto fail;

	if (!json_write_object_end(&jfile))
		goto fail;

	fclose(file);
	return 1;
fail:
	if (file)
		fclose(file);
	return 1;
}

void free_clan(struct clan *clan)
{
	if (clan->members)
		free(clan->members);

	clan->nmembers = 0;
	clan->members = NULL;
}

struct player_info *get_member(struct clan *clan, char *name)
{
	unsigned i;

	for (i = 0; i < clan->nmembers; i++)
		if (!strcmp(clan->members[i].name, name))
			return &clan->members[i];

	return NULL;
}

void remove_member(struct clan *clan, struct player_info *member)
{
	struct player_info *last;

	assert(clan != NULL);
	assert(member - clan->members >= 0);
	assert(member - clan->members < clan->nmembers);

	last = &clan->members[clan->nmembers - 1];

	*member = *last;
	clan->nmembers--;
}

unsigned load_members(struct clan *clan)
{
	unsigned i;
	unsigned removed = 0;
	enum read_player_ret ret;

	for (i = 0; i < clan->nmembers; i++) {
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

	if (c1->nmembers != c2->nmembers)
		return 0;
	if (strcmp(c1->name, c2->name))
		return 0;

	for (i = 0; i < c1->nmembers; i++) {
		for (j = 0; j < c2->nmembers; j++) {
			if (!strcmp(c1->members[i].name, c2->members[j].name))
				break;
		}

		if (j == c2->nmembers)
			return 0;
	}

	return 1;
}
