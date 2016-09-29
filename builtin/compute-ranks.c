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
#include "info.h"

static void set_players_rank(struct index *index)
{
	struct indexed_player *p;
	struct player player;
	unsigned rank = 1;

	init_player(&player);

	while ((p = index_foreach(index))) {
		if (read_player(&player, p->name) == SUCCESS) {
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

static int sort_by_lastseen(const void *pa, const void *pb)
{
	const struct indexed_player *a = pa, *b = pb;

	if (a->lastseen < b->lastseen)
		return 1;
	else if (a->lastseen > b->lastseen)
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

static int is_ctf_vanilla(const void *data)
{
	const struct indexed_server *s = data;

	const char **maps = (const char*[]) {
		"ctf1", "ctf2", "ctf3", "ctf4", "ctf5", "ctf6", "ctf7", NULL
	};

	if (strcmp(s->gametype, "CTF") != 0)
		return 0;

	for (; *maps; maps++)
		if (strcmp(s->map, *maps) == 0)
			return 1;

	return 0;
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
	struct info info;

	load_config(1);

	/* Player index */

	if (!create_index(&index, &INDEX_DATA_INFO_PLAYER, NULL))
		return EXIT_FAILURE;
	info.nplayers = index.ndata;

	/* By elo */
	sort_index(&index, sort_by_elo);
	set_players_rank(&index);

	if (!write_index(&index, "players_by_rank"))
		return EXIT_FAILURE;

	/* By last seen date */
	sort_index(&index, sort_by_lastseen);
	if (!write_index(&index, "players_by_lastseen"))
		return EXIT_FAILURE;

	close_index(&index);

	/* Clan index */

	if (!create_index(&index, &INDEX_DATA_INFO_CLAN, NULL))
		return EXIT_FAILURE;
	info.nclans = index.ndata;

	sort_index(&index, sort_by_nmembers);

	if (!write_index(&index, "clans_by_nmembers"))
		return EXIT_FAILURE;

	close_index(&index);

	/* Server index */

	if (!create_index(&index, &INDEX_DATA_INFO_SERVER, is_ctf_vanilla))
		return EXIT_FAILURE;
	info.nservers = index.ndata;

	sort_index(&index, sort_by_nplayers);

	if (!write_index(&index, "servers_by_nplayers"))
		return EXIT_FAILURE;

	close_index(&index);

	/* Info */

	if (!write_info(&info))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
