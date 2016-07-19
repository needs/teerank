#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "player.h"
#include "index.h"

static void set_players_rank(struct index *index)
{
	struct indexed_player *p;
	struct player player;
	unsigned rank = 1;

	init_player(&player);

	while ((p = index_foreach(index))) {
		if (read_player(&player, p->name) == PLAYER_FOUND) {
			set_rank(&player, rank);
			write_player(&player);
		}

		p->rank = rank;
		rank++;
	}
}

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

int main(int argc, char *argv[])
{
	struct index index;

	load_config(1);

	if (!create_index(&index, INDEX_DATA_INFOS_PLAYER))
		return EXIT_FAILURE;

	sort_index(&index, sort_by_elo);
	set_players_rank(&index);

	if (!write_index(&index, "players_by_rank"))
		return EXIT_FAILURE;

	close_index(&index);

	return EXIT_SUCCESS;
}
