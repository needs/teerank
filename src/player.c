#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "player.h"
#include "config.h"
#include "io.h"
#include "elo.h"

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
}

static int create_player(struct player *player, char *name)
{
	assert(name != NULL);
	assert(player != NULL);

	init_player(player, name);
	player->elo = DEFAULT_ELO;
	player->is_rankable = 0;
	player->is_modified = 1;

	return 1;
}

int read_player(struct player *player, char *name)
{
	char *path;
	int ret;
	FILE *file;

	assert(name != NULL);
	assert(player != NULL);

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

void html_print_player(struct player *player, int show_clan_link)
{
	char tmp[MAX_NAME_LENGTH];

	assert(player != NULL);

	printf("<tr>");

	/* Rank */
	if (player->rank == INVALID_RANK)
		printf("<td>?</td>");
	else
		printf("<td>%u</td>", player->rank);

	/* Name */
	hex_to_string(player->name, tmp);
	printf("<td>"); html(tmp); printf("</td>");

	/* Clan */
	hex_to_string(player->clan, tmp);
	printf("<td>");
	if (*tmp != '\0') {
		if (show_clan_link)
			printf("<a href=\"/clans/%s.html\">", player->clan);
		html(tmp);
		if (show_clan_link)
			printf("</a>");
	}
	printf("</td>");

	/* Elo */
	if (player->elo == INVALID_ELO)
		printf("<td>?</td>");
	else
		printf("<td>%d</td>", player->elo);

	printf("</tr>\n");
}
