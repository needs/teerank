#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

#include "index.h"
#include "config.h"
#include "player.h"
#include "clan.h"

typedef int (*create_index_data_func_t)(void *data, const char *name);
typedef int (*write_index_data_func_t)(void *data, FILE *file, const char *path);
typedef int (*read_index_data_func_t)(void *data, FILE *file, const char *path);

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
	struct player_summary player;
	struct indexed_player *ret = data;

	if (!is_valid_hexname(name))
		return 0;

	if (read_player_summary(&player, name) != PLAYER_FOUND)
		return 0;

	strcpy(ret->name, player.name);
	strcpy(ret->clan, player.clan);
	ret->rank = player.rank;
	ret->elo = player.elo;

	return 1;
}

#define strsize(n) (int)sizeof(#n)

/* Add one because negative ints are prefixed with '-' */
#define INT_STRSIZE (strsize(INT_MIN) + 1)
#define UINT_STRSIZE strsize(UINT_MAX)

static int write_indexed_player(void *data, FILE *file, const char *path)
{
	struct indexed_player *p = data;
	int ret;

	ret = fprintf(
		file, "%-*s %-*s %-*d %-*u\n",
		HEXNAME_LENGTH - 1, p->name, HEXNAME_LENGTH - 1, p->clan,
		INT_STRSIZE, p->elo, UINT_STRSIZE, p->rank);
	if (ret < 0) {
		perror(path);
		return 0;
	}

	return ret;
}

static int read_indexed_player(void *data, FILE *file, const char *path)
{
	struct indexed_player *p = data;
	int ret;

	errno = 0;
	ret = fscanf(file, "%s %s %d %u\n", p->name, p->clan, &p->elo, &p->rank);
	if (ret == EOF && errno != 0) {
		perror(path);
		return 0;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match indexed player name\n", path);
		return 0;
	} else if (ret == 1) {
		fprintf(stderr, "%s: Cannot match indexed player clan\n", path);
		return 0;
	} else if (ret == 2) {
		fprintf(stderr, "%s: Cannot match indexed player elo\n", path);
		return 0;
	} else if (ret == 3) {
		fprintf(stderr, "%s: Cannot match indexed player rank\n", path);
		return 0;
	}

	return 1;
}

const struct index_data_infos *INDEX_DATA_INFOS_PLAYER = &(struct index_data_infos) {
	"players",
	sizeof(struct indexed_player),
	HEXNAME_LENGTH - 1 + HEXNAME_LENGTH - 1 + INT_STRSIZE + UINT_STRSIZE + 4,
	create_indexed_player,
	write_indexed_player,
	read_indexed_player
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

	unsigned i;
	FILE *file = 0;
	int ret;

	ret = snprintf(path, PATH_MAX, "%s/%s", config.root, filename);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	if (!(file = fopen(path, "w"))) {
		perror(path);
		goto fail;
	}

	/* Header */
	ret = fprintf(file, "%u entries\n", index->ndata);
	if (ret < 0) {
		perror(path);
		goto fail;
	}

	/* Data */
	for (i = 0; i < index->ndata; i++) {
		ret = index->infos->write_data(get(index, i), file, path);
		if (ret < 0) {
			perror(path);
			goto fail;
		} else if (ret != index->infos->entry_size) {
			fprintf(stderr, "%s: Bytes written do not match entry size\n", path);
			goto fail;
		}
	}

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
	int ret;
	unsigned ndata;

	assert(ipage != NULL);
	assert(infos != NULL);
	assert(plen > 0);
	assert(pnum > 0);

	ipage->file = NULL;

	ret = snprintf(ipage->path, PATH_MAX, "%s/%s", config.root, filename);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	if (!(ipage->file = fopen(ipage->path, "r"))) {
		perror(ipage->path);
		goto fail;
	}

	/* First, read header */
	errno = 0;
	ret = fscanf(ipage->file, "%u entries\n", &ndata);
	if (ret == EOF && errno != 0) {
		perror(ipage->path);
		goto fail;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match header\n", ipage->path);
		goto fail;
	}

	ipage->npages = ndata / plen + 1;

	if (pnum > ipage->npages) {
		fprintf(stderr, "Only %u pages available\n", ipage->npages);
		goto not_found;
	}

	if (fseek(ipage->file, infos->entry_size * (pnum - 1) * plen, SEEK_CUR) == -1) {
		perror(ipage->path);
		goto fail;
	}

	ipage->infos = infos;
	ipage->ndata = ndata;
	ipage->pnum = pnum;
	ipage->plen = plen;
	ipage->i = 0;

	return PAGE_FOUND;

fail:
	if (ipage->file)
		fclose(ipage->file);
	return PAGE_ERROR;

not_found:
	fclose(ipage->file);
	return PAGE_NOT_FOUND;
}

int index_page_foreach(struct index_page *ipage, void *data)
{
	assert(ipage != NULL);
	assert(ipage->file != NULL);
	assert(ipage->i <= ipage->plen);

	if (ipage->i == ipage->plen)
		return 0;

	ipage->i += 1;
	return ipage->infos->read_data(data, ipage->file, ipage->path);
}

void close_index_page(struct index_page *ipage)
{
	assert(ipage != NULL);
	assert(ipage->file != NULL);

	fclose(ipage->file);
}
