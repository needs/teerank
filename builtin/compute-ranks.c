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

static int sort_by_last_seen(const void *pa, const void *pb)
{
	const struct indexed_player *a = pa, *b = pb;

	if (a->last_seen < b->last_seen)
		return 1;
	else if (a->last_seen > b->last_seen)
		return -1;

	/* Fall back to elo if times are the same */
	if (a->elo < b->elo)
		return 1;
	else if (a->elo == b->elo)
		return 0;
	else
		return -1;
}

static int sort_by_nmembers(const void *pa, const void *pb)
{
	const struct indexed_clan *a = pa, *b = pb;

	if (a->nmembers < b->nmembers)
		return 1;
	else if (a->nmembers > b->nmembers)
		return -1;

	return strcmp(a->name, b->name);
}

static int sort_by_nplayers(const void *pa, const void *pb)
{
	const struct indexed_server *a = pa, *b = pb;

	if (a->nplayers < b->nplayers)
		return 1;
	else if (a->nplayers > b->nplayers)
		return -1;

	return strcmp(a->name, b->name);
}

int main(int argc, char *argv[])
{
	struct index index;

	load_config(1);

	/* Player index */

	if (!create_index(&index, INDEX_DATA_INFOS_PLAYER))
		return EXIT_FAILURE;

	/* By elo */
	sort_index(&index, sort_by_elo);
	set_players_rank(&index);

	if (!write_index(&index, "players_by_rank"))
		return EXIT_FAILURE;

	/* By last seen date */
	sort_index(&index, sort_by_last_seen);
	if (!write_index(&index, "players_by_last_seen"))
		return EXIT_FAILURE;

	close_index(&index);

	/* Clan index */

	if (!create_index(&index, INDEX_DATA_INFOS_CLAN))
		return EXIT_FAILURE;

	sort_index(&index, sort_by_nmembers);

	if (!write_index(&index, "clans_by_nmembers"))
		return EXIT_FAILURE;

	close_index(&index);

	/* Server index */

	if (!create_index(&index, INDEX_DATA_INFOS_SERVER))
		return EXIT_FAILURE;

	sort_index(&index, sort_by_nplayers);

	if (!write_index(&index, "servers_by_nplayers"))
		return EXIT_FAILURE;

	close_index(&index);

	return EXIT_SUCCESS;
}
