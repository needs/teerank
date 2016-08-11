#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include "config.h"
#include "index.h"

#include "teerank5/core/player.h"

static int sort_by_elo(const void *pa, const void *pb)
{
	const struct indexed_player *a = pa, *b = pb;

	if (a->elo < b->elo)
		return 1;
	else if (a->elo == b->elo)
		return 0;
	else
		return -1;
}

static void create_players_by_rank_file(void)
{
	struct index index;

	if (!create_index(&index, INDEX_DATA_INFOS_PLAYER))
		exit(EXIT_FAILURE);

	sort_index(&index, sort_by_elo);

	if (!write_index(&index, "players_by_rank")) {
		close_index(&index);
		exit(EXIT_FAILURE);
	}

	close_index(&index);
}

static void remove_ranks_file(void)
{
	char path[PATH_MAX];

	if (!dbpath(path, PATH_MAX, "ranks"))
		exit(EXIT_FAILURE);

	if (unlink(path) == -1 && errno != ENOENT) {
		perror(path);
		exit(EXIT_FAILURE);
	}
}

static void upgrade_historic(struct teerank5_historic *old, struct historic *new)
{
	new->epoch = old->epoch;
	new->nrecords = old->nrecords;
	new->max_records = old->max_records;
	new->length = old->length;
	new->records = (struct record*)old->records;
	new->first = (struct record*)old->first;
	new->last = (struct record*)old->last;
	new->data_size = old->data_size;
	new->data = old->data;
}

static void upgrade_player(struct teerank5_player *old, struct player *new)
{
	memcpy(new->name, old->name, sizeof(new->name));
	memcpy(new->clan, old->clan, sizeof(new->clan));

	new->elo = old->elo;
	new->rank = old->rank;

	/*
	 * Historic structs hasn't changed at all, so it should be safe
	 * to copy them like that.
	 */
	upgrade_historic(&old->hist, &new->hist);

	/* The only change is the added last_seen field */
	new->last_seen = *gmtime(&NEVER_SEEN);
}

static void upgrade_players(void)
{
	struct teerank5_player old;
	struct player new;

	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	if (!dbpath(path, PATH_MAX, "players"))
		exit(EXIT_FAILURE);

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	teerank5_init_player(&old);
	init_player(&new);

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		teerank5_read_player(&old, dp->d_name);
		upgrade_player(&old, &new);
		write_player(&new);
	}

	closedir(dir);
}

int main(int argc, char *argv[])
{
	load_config(0);

	remove_ranks_file();
	upgrade_players();
	create_players_by_rank_file();

	return EXIT_SUCCESS;
}
