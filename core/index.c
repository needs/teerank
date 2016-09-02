#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

#include "server.h"
#include "index.h"
#include "config.h"
#include "player.h"
#include "clan.h"

typedef int (*create_index_data_func_t)(void *data, const char *name);
typedef int (*write_index_data_func_t)(struct jfile *jfile, const void *data);
typedef int (*read_index_data_func_t)(struct jfile *jfile, void *data);

struct index_data_infos {
	const char *dirname;

	size_t size;
	size_t entry_size;

	create_index_data_func_t create_data;
	write_index_data_func_t write_data;
	read_index_data_func_t read_data;
};

/*
 * INDEX_DATA_INFOS_PLAYER
 */

static int create_indexed_player(void *data, const char *name)
{
	struct player_info player;
	struct indexed_player *ret = data;

	if (!is_valid_hexname(name))
		return 0;

	if (read_player_info(&player, name) != PLAYER_FOUND)
		return 0;

	strcpy(ret->name, player.name);
	strcpy(ret->clan, player.clan);
	ret->rank = player.rank;
	ret->elo = player.elo;
	ret->last_seen = player.last_seen;

	return 1;
}

static int write_indexed_player(struct jfile *jfile, const void *data)
{
	const struct indexed_player *player = data;

	if (!json_write_string(jfile, NULL, player->name, sizeof(player->name)))
		return 0;
	if (!json_write_string(jfile, NULL, player->clan, sizeof(player->clan)))
		return 0;
	if (!json_write_int(jfile, NULL, player->elo))
		return 0;
	if (!json_write_unsigned(jfile, NULL, player->rank))
		return 0;
	if (!json_write_tm(jfile, NULL, player->last_seen))
		return 0;

	return 1;
}

static int read_indexed_player(struct jfile *jfile, void *data)
{
	struct indexed_player *player = data;

	if (!json_read_string(jfile, NULL, player->name, sizeof(player->name)))
		return 0;
	if (!json_read_string(jfile, NULL, player->clan, sizeof(player->clan)))
		return 0;
	if (!json_read_int(jfile, NULL, &player->elo))
		return 0;
	if (!json_read_unsigned(jfile, NULL, &player->rank))
		return 0;
	if (!json_read_tm(jfile, NULL, &player->last_seen))
		return 0;

	return 1;
}

const struct index_data_infos *INDEX_DATA_INFOS_PLAYER = &(struct index_data_infos) {
	"players",
	sizeof(struct indexed_player),
	JSON_ARRAY_SIZE +
	JSON_RAW_STRING_SIZE(HEXNAME_LENGTH) + /* Name */
	JSON_RAW_STRING_SIZE(HEXNAME_LENGTH) + /* Clan */
	JSON_INT_SIZE +                        /* Elo */
	JSON_UINT_SIZE +                       /* Rank */
	JSON_DATE_SIZE,                        /* Last seen */
	create_indexed_player,
	write_indexed_player,
	read_indexed_player
};

/*
 * INDEX_DATA_INFOS_CLAN
 */

static int create_indexed_clan(void *data, const char *name)
{
	struct clan clan;
	struct indexed_clan *ret = data;

	if (!is_valid_hexname(name))
		return 0;

	if (read_clan(&clan, name) != CLAN_FOUND)
		return 0;

	strcpy(ret->name, clan.name);
	ret->nmembers = clan.nmembers;

	return 1;
}

static int write_indexed_clan(struct jfile *jfile, const void *data)
{
	const struct indexed_clan *clan = data;

	if (!json_write_string(jfile, NULL, clan->name, sizeof(clan->name)))
		return 0;
	if (!json_write_unsigned(jfile, NULL, clan->nmembers))
		return 0;

	return 1;
}

static int read_indexed_clan(struct jfile *jfile, void *data)
{
	struct indexed_clan *clan = data;

	if (!json_read_string(jfile, NULL, clan->name, sizeof(clan->name)))
		return 0;
	if (!json_read_unsigned(jfile, NULL, &clan->nmembers))
		return 0;

	return 1;
}

const struct index_data_infos *INDEX_DATA_INFOS_CLAN = &(struct index_data_infos) {
	"clans",
	sizeof(struct indexed_clan),
	JSON_ARRAY_SIZE +
	JSON_RAW_STRING_SIZE(HEXNAME_LENGTH) + /* Name */
	JSON_UINT_SIZE,                        /* Number of members */
	create_indexed_clan,
	write_indexed_clan,
	read_indexed_clan
};

/*
 * INDEX_DATA_INFOS_SERVER
 */

static int create_indexed_server(void *data, const char *name)
{
	struct server_state server;
	struct indexed_server *ret = data;

	if (!read_server_state(&server, name))
		return 0;

	strcpy(ret->name, server.name);
	strcpy(ret->gametype, server.gametype);
	strcpy(ret->map, server.map);
	ret->nplayers = server.num_clients;
	ret->maxplayers = server.max_clients;

	return 1;
}

static int write_indexed_server(struct jfile *jfile, const void *data)
{
	const struct indexed_server *server = data;

	if (!json_write_string(jfile, NULL, server->name, sizeof(server->name)))
		return 0;
	if (!json_write_string(jfile, NULL, server->gametype, sizeof(server->gametype)))
		return 0;
	if (!json_write_string(jfile, NULL, server->map, sizeof(server->map)))
		return 0;
	if (!json_write_unsigned(jfile, NULL, server->nplayers))
		return 0;
	if (!json_write_unsigned(jfile, NULL, server->maxplayers))
		return 0;

	return 1;
}

static int read_indexed_server(struct jfile *jfile, void *data)
{
	struct indexed_server *server = data;

	if (!json_read_string(jfile, NULL, server->name, sizeof(server->name)))
		return 0;
	if (!json_read_string(jfile, NULL, server->gametype, sizeof(server->gametype)))
		return 0;
	if (!json_read_string(jfile, NULL, server->map, sizeof(server->map)))
		return 0;
	if (!json_read_unsigned(jfile, NULL, &server->nplayers))
		return 0;
	if (!json_read_unsigned(jfile, NULL, &server->maxplayers))
		return 0;

	return 1;
}

const struct index_data_infos *INDEX_DATA_INFOS_SERVER = &(struct index_data_infos) {
	"servers",
	sizeof(struct indexed_server),
	JSON_ARRAY_SIZE +
	JSON_STRING_SIZE(SERVERNAME_STRSIZE) + /* Server name */
	JSON_STRING_SIZE(GAMETYPE_STRSIZE) +   /* Gametype */
	JSON_STRING_SIZE(MAP_STRSIZE) +        /* Map */
	JSON_UINT_SIZE +                       /* Number of players */
	JSON_UINT_SIZE,                        /* Max number of players */
	create_indexed_server,
	write_indexed_server,
	read_indexed_server
};

/*
 * Since data are of type void, we must compute the right offset using
 * data_size to get the data.
 */
static void *get(struct index *index, unsigned i)
{
	assert(index != NULL);
	assert(i < index->ndata);

	return (char*)index->data + (index->infos->size * i);
}

static void *new_data(struct index *index)
{
	/*
	 * It tooks 4 months to get to 70k players, so in average, there
	 * will be one additional realloc() every years.
	 */
	static const unsigned STEP = 200000;

	/*
	 * When index->reuse_last is set, it means the last data that was
	 * allocated is in fact unused, so we don't have to do all the
	 * realloc() stuff: we can just reuse the last data.
	 */
	if (index->reuse_last) {
		assert(index->ndata > 0);

		index->reuse_last = 0;
		return get(index, index->ndata - 1);
	}

	if (index->ndata % STEP == 0) {
		void *tmp;

		tmp = realloc(index->data, (index->ndata + STEP) * index->infos->size);
		if (!tmp) {
			fprintf(stderr, "realloc(%u): %s\n",
			        index->ndata, strerror(errno));
			return NULL;
		}
		index->data = tmp;
	}

	index->ndata += 1;
	return get(index, index->ndata - 1);
}

static void free_last_data(struct index *index)
{
	assert(index != NULL);
	assert(index->reuse_last == 0);
	assert(index->ndata > 0);

	/*
	 * Freeing is just marking the last allocated data as reusable
	 * so we don't have to reallocate it.  Decreasing index->ndata
	 * will not work because it breaks the assumption that the index
	 * is always growing.
	 */
	index->reuse_last = 1;
}

int create_index(struct index *index, const struct index_data_infos *infos)
{
	static const struct index INDEX_ZERO;
	static char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	assert(index != NULL);
	assert(infos != NULL);

	*index = INDEX_ZERO;
	index->infos = infos;

	sprintf(path, "%s/%s", config.root, infos->dirname);
	if (!(dir = opendir(path))) {
		perror(path);
		return 0;
	}

	while ((dp = readdir(dir))) {
		void *data;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		data = new_data(index);

		/*
		 * Failing to create data is fatal, because it probably
		 * means not enough memory is available, and we don't
		 * want a truncated index.
		 */
		if (!data) {
			closedir(dir);
			return 0;
		}

		if (!infos->create_data(data, dp->d_name)) {
			free_last_data(index);
			continue;
		}
	}

	closedir(dir);

	return 1;
}

void sort_index(struct index *index, int (*compar)(const void *, const void *))
{
	qsort(index->data, index->ndata, index->infos->size, compar);
}

void *index_foreach(struct index *index)
{
	if (index->i == index->ndata) {
		index->i = 0;
		return NULL;
	}

	index->i += 1;
	return get(index, index->i - 1);
}

int write_index(struct index *index, const char *filename)
{
	char path[PATH_MAX];
	FILE *file = NULL;
	struct jfile jfile;

	unsigned i;

	if (!dbpath(path, PATH_MAX, "%s", filename))
		goto fail;

	if (!(file = fopen(path, "w"))) {
		perror(path);
		goto fail;
	}

	json_init(&jfile, file, path);

	if (!json_write_object_start(&jfile, NULL))
		goto fail;

	/* Header */
	if (!json_write_unsigned(&jfile, "nentries", index->ndata))
		goto fail;

	if (!json_write_indexable_array_start(&jfile, "entries", index->infos->entry_size))
		goto fail;

	/* Entries */
	for (i = 0; i < index->ndata; i++) {
		if (!json_write_array_start(&jfile, NULL))
			goto fail;
		if (!index->infos->write_data(&jfile, get(index, i)))
			goto fail;
		if (!json_write_array_end(&jfile))
			goto fail;
	}

	if (!json_write_indexable_array_end(&jfile))
		goto fail;

	if (!json_write_object_end(&jfile))
		goto fail;

	fclose(file);

	return 1;

fail:
	if (file)
		fclose(file);
	return 0;
}

void close_index(struct index *index)
{
	assert(index != NULL);
	assert(index != NULL);

	free(index->data);
}

int open_index_page(
	const char *filename,
	struct index_page *ipage, const struct index_data_infos *infos,
	unsigned pnum, unsigned plen)
{
	unsigned ndata;

	assert(ipage != NULL);
	assert(infos != NULL);
	assert((plen == 0 && pnum == 1) || (plen > 0 && pnum > 0));

	ipage->file = NULL;

	if (!dbpath(ipage->path, sizeof(ipage->path), "%s", filename))
		goto fail;

	if (!(ipage->file = fopen(ipage->path, "r"))) {
		perror(ipage->path);
		goto fail;
	}

	json_init(&ipage->jfile, ipage->file, ipage->path);

	/*
	 * First, skip header so that seeking the file does use the right
	 * offset to begin with.
	 */
	if (!json_read_object_start(&ipage->jfile, NULL))
		goto fail;
	if (!json_read_unsigned(&ipage->jfile, "nentries", &ndata))
		goto fail;
	if (!json_read_indexable_array_start(&ipage->jfile, "entries", infos->entry_size))
		goto fail;

	if (plen) {
		ipage->npages = ndata / plen + 1;
	} else {
		ipage->npages = 1;
		plen = ndata;
	}

	if (pnum > ipage->npages) {
		fprintf(stderr, "Only %u pages available\n", ipage->npages);
		goto not_found;
	}

	if (!json_seek_indexable_array(&ipage->jfile, (pnum - 1) * plen))
		goto fail;

	ipage->infos = infos;
	ipage->ndata = ndata;
	ipage->pnum = pnum;
	ipage->i = 0;

	/*
	 * If it is the last page, page length may be lower than
	 * standard page length.
	 */
	if (pnum == ipage->npages)
		ipage->plen = ndata % plen;
	else
		ipage->plen = plen;

	return PAGE_FOUND;

fail:
	if (ipage->file)
		fclose(ipage->file);
	return PAGE_ERROR;

not_found:
	fclose(ipage->file);
	return PAGE_NOT_FOUND;
}

unsigned index_page_foreach(struct index_page *ipage, void *data)
{
	assert(ipage != NULL);
	assert(ipage->file != NULL);
	assert(ipage->i <= ipage->plen);

	if (ipage->i == ipage->plen) {
		ipage->i = 0;
		return 0;
	}

	ipage->i += 1;

	if (!json_read_array_start(&ipage->jfile, NULL))
		return 0;
	if (ipage->infos->read_data(&ipage->jfile, data) == 0)
		return 0;
	if (!json_read_array_end(&ipage->jfile))
		return 0;

	return (ipage->pnum - 1) * ipage->plen + ipage->i;
}

void close_index_page(struct index_page *ipage)
{
	assert(ipage != NULL);
	assert(ipage->file != NULL);

	fclose(ipage->file);
}
