#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "server.h"
#include "index.h"
#include "config.h"
#include "player.h"
#include "clan.h"

/*
 * INDEX_DATA_INFO_PLAYER
 */

static int create_indexed_player(void *data, const char *name)
{
	struct player_info player;
	struct indexed_player *ret = data;

	if (!is_valid_hexname(name))
		return 0;

	if (read_player_info(&player, name) != SUCCESS)
		return 0;

	strcpy(ret->name, player.name);
	strcpy(ret->clan, player.clan);
	ret->rank = player.rank;
	ret->elo = player.elo;
	ret->lastseen = mktime(&player.lastseen);
	strcpy(ret->server_ip, player.server_ip);
	strcpy(ret->server_port, player.server_port);

	return 1;
}

const struct index_data_info INDEX_DATA_INFO_PLAYER = {
	"players",
	sizeof(struct indexed_player),
	create_indexed_player
};

/*
 * INDEX_DATA_INFO_CLAN
 */

static int create_indexed_clan(void *data, const char *name)
{
	struct clan clan;
	struct indexed_clan *ret = data;

	if (!is_valid_hexname(name))
		return 0;

	if (read_clan(&clan, name) != SUCCESS)
		return 0;

	strcpy(ret->name, clan.name);
	ret->nmembers = clan.nmembers;

	return 1;
}

const struct index_data_info INDEX_DATA_INFO_CLAN = {
	"clans",
	sizeof(struct indexed_clan),
	create_indexed_clan
};

/*
 * INDEX_DATA_INFO_SERVER
 */

static int create_indexed_server(void *data, const char *name)
{
	struct server server;
	struct indexed_server *ret = data;

	if (read_server(&server, name) != SUCCESS)
		return 0;

	strcpy(ret->ip, server.ip);
	strcpy(ret->port, server.port);

	strcpy(ret->name, server.name);
	strcpy(ret->gametype, server.gametype);
	strcpy(ret->map, server.map);
	ret->nplayers = server.num_clients;
	ret->maxplayers = server.max_clients;

	return 1;
}

const struct index_data_info INDEX_DATA_INFO_SERVER = {
	"servers",
	sizeof(struct indexed_server),
	create_indexed_server
};

/*
 * Since data are of type void, we must compute the right offset using
 * data_size to get the data.
 */
static void *get(struct index *index, unsigned i)
{
	assert(index != NULL);
	assert(i < index->ndata);

	return (char*)index->data + (index->info->datasize * i);
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

		tmp = realloc(index->data, (index->ndata + STEP) * index->info->datasize);
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

int create_index(
	struct index *index, const struct index_data_info *info,
	int (*filter)(const void *))
{
	static const struct index INDEX_ZERO;
	static char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	assert(index != NULL);
	assert(info != NULL);

	*index = INDEX_ZERO;
	index->info = info;

	if (!dbpath(path, PATH_MAX, "%s", info->dirname))
		return 0;

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

		if (!info->create_data(data, dp->d_name)) {
			free_last_data(index);
			continue;
		}

		if (filter && filter(data) == 0) {
			free_last_data(index);
			continue;
		}
	}

	closedir(dir);

	return 1;
}

/*
 * If index_reuse_last is true, we need to ignore the last data:
 * reuse_last means the data is counted within index->ndata but is
 * actually a dummy entry.
 */
unsigned index_ndata(struct index *index)
{
	return index->ndata - index->reuse_last;
}

void sort_index(struct index *index, int (*compar)(const void *, const void *))
{
	qsort(index->data, index_ndata(index), index->info->datasize, compar);
}

void *index_foreach(struct index *index)
{
	if (index->i == index_ndata(index)) {
		index->i = 0;
		return NULL;
	}

	index->i += 1;
	return get(index, index->i - 1);
}

int write_index(struct index *index, const char *filename)
{
	char path[PATH_MAX];
	int fd = -1;
	ssize_t ret;
	unsigned ndata;
	size_t totalsize;

	if (!dbpath(path, PATH_MAX, "%s", filename))
		goto fail;

	if ((fd = open(path, O_WRONLY | O_CREAT, (mode_t)0666)) == -1) {
		perror(path);
		goto fail;
	}

	/* ndata */
	ndata = index_ndata(index);
	ret = write(fd, &ndata, sizeof(ndata));

	if (ret == -1) {
		perror(path);
		goto fail;
	} else if (ret < sizeof(index->ndata)) {
		fprintf(stderr, "%s: write(): Header truncated\n", path);
		goto fail;
	}

	/* Entries */
	totalsize = index->info->datasize * ndata;
	ret = write(fd, index->data, totalsize);

	if (ret == -1) {
		perror(path);
		goto fail;
	} else if (ret < totalsize) {
		fprintf(stderr, "%s: write(): Entries truncated\n", path);
		goto fail;
	}

	close(fd);

	return 1;

fail:
	if (fd >= 0)
		close(fd);
	return 0;
}

void close_index(struct index *index)
{
	assert(index != NULL);
	assert(index != NULL);

	free(index->data);
}

static int select_page(
	struct index_page *ipage, unsigned pnum, unsigned plen)
{
	size_t entryoffset;

	/*
	 * If there is no page length, then we treats the whole index as
	 * one single page.
	 */
	if (!plen)
		ipage->npages = 1;
	else
		ipage->npages = ipage->ndata / plen + 1;

	if (pnum > ipage->npages) {
		fprintf(stderr, "Only %u pages available\n", ipage->npages);
		return NOT_FOUND;
	}

	entryoffset = (pnum - 1) * plen * ipage->info->datasize;
	ipage->data = ipage->mmapbuf + sizeof(ipage->ndata) + entryoffset;

	/*
	 * If it is the last page, page length may be lower than given
	 * page length.
	 */
	if (!plen)
		ipage->plen = ipage->ndata;
	else if (pnum == ipage->npages)
		ipage->plen = ipage->ndata % plen;
	else
		ipage->plen = plen;

	ipage->istart = (pnum - 1) * plen;
	ipage->pnum = pnum;
	ipage->i = 0;

	return SUCCESS;
}

int open_index_page(
	const char *filename,
	struct index_page *ipage, const struct index_data_info *info,
	unsigned pnum, unsigned plen)
{
	ssize_t ret;

	assert(ipage != NULL);
	assert(info != NULL);
	assert((plen == 0 && pnum == 1) || (plen > 0 && pnum > 0));

	ipage->fd = -1;
	ipage->mmapbuf = MAP_FAILED;
	ipage->info = info;

	if (!dbpath(ipage->path, sizeof(ipage->path), "%s", filename))
		goto fail;

	if (!(ipage->fd = open(ipage->path, O_RDONLY))) {
		perror(ipage->path);
		goto fail;
	}

	/*
	 * First, get the number of data, so that the filesize can be
	 * computed.  Reading before mapping is no problem, it is two
	 * independant operations.
	 */
	ret = read(ipage->fd, &ipage->ndata, sizeof(ipage->ndata));

	if (ret == -1) {
		perror(ipage->path);
		goto fail;
	} else if (ret < sizeof(ipage->ndata)) {
		fprintf(stderr, "%s: read(): Header truncated\n", ipage->path);
		goto fail;
	}

	/*
	 * Map the file in memory.  Using PROT_READ | PROT_WRITE with
	 * MAP_PRIVATE works even if the file has been previously
	 * openened with O_RDONLY.
	 */
	ipage->filesize = sizeof(ipage->ndata) + ipage->ndata * ipage->info->datasize;
	ipage->mmapbuf = mmap(0, ipage->filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE, ipage->fd, 0);

	if (ipage->mmapbuf == MAP_FAILED) {
		perror(ipage->path);
		goto fail;
	}

	switch (select_page(ipage, pnum, plen)) {
	case NOT_FOUND:
		goto not_found;
	case FAILURE:
		goto fail;
	}

	return SUCCESS;

fail:
	if (ipage->fd >= 0)
		close(ipage->fd);
	if (ipage->mmapbuf == MAP_FAILED)
		munmap(ipage->mmapbuf, ipage->filesize);
	return FAILURE;

not_found:
	close(ipage->fd);
	return NOT_FOUND;
}

void *index_page_foreach(struct index_page *ipage, unsigned *i)
{
	assert(ipage != NULL);
	assert(ipage->fd != -1);
	assert(ipage->i <= ipage->plen);

	if (ipage->i == ipage->plen) {
		ipage->i = 0;
		return NULL;
	}

	if (i)
		*i = ipage->istart + ipage->i + 1;

	return ipage->data + (ipage->i++ * ipage->info->datasize);
}

void close_index_page(struct index_page *ipage)
{
	assert(ipage != NULL);
	assert(ipage->fd != -1);

	munmap(ipage->mmapbuf, ipage->filesize);
	close(ipage->fd);
}
