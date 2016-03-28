#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "io.h"
#include "config.h"
#include "player.h"

/*
 * Create a temporary stream pointing to "path".
 *
 * "path" must be suitable for a call to mkstemp().
 */
static FILE *fopen_tmp(char *path)
{
	int fd;
	FILE *file;

	assert(path != NULL);
	assert(strlen(path) >= 6);
	assert(!strcmp(path + strlen(path) - 6, "XXXXXX"));

	if ((fd = mkstemp(path)) == -1) {
		fprintf(stderr, "mkstemp(\"%s\"): %s\n", path, strerror(errno));
		return NULL;
	}
	if (!(file = fdopen(fd, "w"))) {
		fprintf(stderr, "fdopen(\"%s\"): %s\n", path, strerror(errno));
		return NULL;
	}

	return file;
}

static int clan_remove_player(char *clan, char *player)
{
	int ret;
	FILE *in, *out;
	char name[MAX_NAME_HEX_LENGTH];
	static char old[PATH_MAX], new[PATH_MAX];

	ret = snprintf(old, PATH_MAX, "%s/clans/%s", config.root, clan);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(old, %d): Too long\n", PATH_MAX);
		return 0;
	}
	ret = snprintf(new, PATH_MAX, "%s/clans/%s-tmp-XXXXXX",
	               config.root, clan);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(new, %d): Too long\n", PATH_MAX);
		return 0;
	}

	if (!(in = fopen(old, "r"))) {
		if (errno != ENOENT)
			return perror(old), 0;
		else
			return 1;
	}
	if (!(out = fopen_tmp(new)))
		return fclose(in), 0;

	while ((fscanf(in, "%s", name)) == 1)
		if (strcmp(name, player))
			fprintf(out, "%s\n", name);

	fclose(in);
	fclose(out);

	if (rename(new, old) == -1) {
		fprintf(stderr, "rename(\"%s\", \"%s\"): %s\n",
		        old, new, strerror(errno));
		return unlink(new), 0;
	}

	return 1;
}

static int clan_add_player(char *clan, char *player)
{
	int ret;
	FILE *in = NULL, *out;
	char name[MAX_NAME_HEX_LENGTH];
	static char old[PATH_MAX], new[PATH_MAX];

	ret = snprintf(old, PATH_MAX, "%s/clans/%s", config.root, clan);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(old, %d): Too long\n", PATH_MAX);
		return 0;
	}
	ret = snprintf(new, PATH_MAX, "%s/clans/%s-tmp-XXXXXX",
	               config.root, clan);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(new, %d): Too long\n", PATH_MAX);
		return 0;
	}

	if (!(out = fopen_tmp(new)))
		return fclose(in), 0;
	if (!(in = fopen(old, "r")) && errno != ENOENT) {
		return perror(old), 0;
	} else if (in) {
		while ((fscanf(in, "%s", name)) == 1)
			fprintf(out, "%s\n", name);
		fclose(in);
	}

	fprintf(out, "%s\n", player);
	fclose(out);

	if (rename(new, old) == -1) {
		fprintf(stderr, "rename(\"%s\", \"%s\"): %s\n",
		        old, new, strerror(errno));
		return unlink(new), 0;
	}

	return 1;
}

/*
 * Remove "player" from "src_clan" and add it to "dest_clan".
 *
 * This function does the best to make sure if one step fail, the database
 * remain in a consistent state.
 */
static int clan_move_player(char *src_clan, char *dest_clan, char *player)
{
	assert(src_clan != NULL);
	assert(dest_clan != NULL);
	assert(player != NULL);
	assert(strcmp(src_clan, dest_clan));

	/*
	 * Add the player first to make sure the player is referenced by
	 * at least one clan at any time.
	 */
	if (strcmp(dest_clan, "00"))
		if (!clan_add_player(dest_clan, player))
			return 0;

	if (strcmp(src_clan, "00"))
		if (!clan_remove_player(src_clan, player))
			return 0;

	return 1;
}

int main(int argc, char *argv[])
{
	int ret;
	char player[MAX_NAME_HEX_LENGTH];
	char    old[MAX_CLAN_HEX_LENGTH];
	char    new[MAX_CLAN_HEX_LENGTH];

	load_config();

	while ((ret = scanf(" %s %s %s", player, old, new)) == 3) {
		if (!strcmp(old, new)) {
			fprintf(stderr, "scanf(delta): Old and new clan must be different (%s)\n", old);
			continue;
		}
		clan_move_player(old, new, player);
	}

	if (ret == EOF && !ferror(stdin))
		return EXIT_SUCCESS;
	else if (ferror(stdin))
		perror("scanf(delta)");
	else if (ret == 0)
		fprintf(stderr, "scanf(delta): Cannot match player name\n");
	else if (ret == 1)
		fprintf(stderr, "scanf(delta): Cannot match old clan\n");
	else if (ret == 2)
		fprintf(stderr, "scanf(delta): Cannot match new clan\n");

	return EXIT_FAILURE;
}
