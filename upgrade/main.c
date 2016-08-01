#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "index.h"

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

int main(int argc, char *argv[])
{
	load_config(0);

	remove_ranks_file();
	create_players_by_rank_file();

	return EXIT_SUCCESS;
}
