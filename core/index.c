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

static void write_indexed_player(struct jfile *jfile, const void *data)
{
	const struct indexed_player *player = data;

	json_write_string(  jfile, NULL, player->name, sizeof(player->name));
	json_write_string(  jfile, NULL, player->clan, sizeof(player->clan));
	json_write_int(     jfile, NULL, player->elo);
	json_write_unsigned(jfile, NULL, player->rank);
	json_write_time(    jfile, NULL, player->lastseen);
	json_write_string(  jfile, NULL, player->server_ip,   sizeof(player->server_ip));
	json_write_string(  jfile, NULL, player->server_port, sizeof(player->server_port));
}

static void read_indexed_player(struct jfile *jfile, void *data)
{
	struct indexed_player *player = data;

	json_read_string(  jfile, NULL, player->name, sizeof(player->name));
	json_read_string(  jfile, NULL, player->clan, sizeof(player->clan));
	json_read_int(     jfile, NULL, &player->elo);
	json_read_unsigned(jfile, NULL, &player->rank);
	json_read_time(    jfile, NULL, &player->lastseen);
	json_read_string(  jfile, NULL, player->server_ip,   sizeof(player->server_ip));
	json_read_string(  jfile, NULL, player->server_port, sizeof(player->server_port));
}

const struct index_data_info INDEX_DATA_INFO_PLAYER = {
	"players",
	sizeof(struct indexed_player),
	JSON_ARRAY_SIZE +
	JSON_RAW_STRING_SIZE(HEXNAME_LENGTH) + /* Name */
	JSON_RAW_STRING_SIZE(HEXNAME_LENGTH) + /* Clan */
	JSON_INT_SIZE +                        /* Elo */
	JSON_UINT_SIZE +                       /* Rank */
	JSON_DATE_SIZE +                       /* Last seen */
	JSON_RAW_STRING_SIZE(IP_STRSIZE) +     /* Server IP */
	JSON_RAW_STRING_SIZE(PORT_STRSIZE),    /* Server port */
	create_indexed_player,
	write_indexed_player,
	read_indexed_player
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

static void write_indexed_clan(struct jfile *jfile, const void *data)
{
	const struct indexed_clan *clan = data;

	json_write_string(jfile, NULL, clan->name, sizeof(clan->name));
	json_write_unsigned(jfile, NULL, clan->nmembers);
}

static void read_indexed_clan(struct jfile *jfile, void *data)
{
	struct indexed_clan *clan = data;

	json_read_string(  jfile, NULL, clan->name, sizeof(clan->name));
	json_read_unsigned(jfile, NULL, &clan->nmembers);
}

const struct index_data_info INDEX_DATA_INFO_CLAN = {
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

static void write_indexed_server(struct jfile *jfile, const void *data)
{
	const struct indexed_server *server = data;

	json_write_string(  jfile, NULL, server->ip,   sizeof(server->ip));
	json_write_string(  jfile, NULL, server->port, sizeof(server->port));

	json_write_string(  jfile, NULL, server->name,     sizeof(server->name));
	json_write_string(  jfile, NULL, server->gametype, sizeof(server->gametype));
	json_write_string(  jfile, NULL, server->map,      sizeof(server->map));
	json_write_unsigned(jfile, NULL, server->nplayers);
	json_write_unsigned(jfile, NULL, server->maxplayers);
}

static void read_indexed_server(struct jfile *jfile, void *data)
{
	struct indexed_server *server = data;

	json_read_string(  jfile, NULL, server->ip,   sizeof(server->ip));
	json_read_string(  jfile, NULL, server->port, sizeof(server->port));

	json_read_string(  jfile, NULL, server->name,     sizeof(server->name));
	json_read_string(  jfile, NULL, server->gametype, sizeof(server->gametype));
	json_read_string(  jfile, NULL, server->map,      sizeof(server->map));
	json_read_unsigned(jfile, NULL, &server->nplayers);
	json_read_unsigned(jfile, NULL, &server->maxplayers);
}

const struct index_data_info INDEX_DATA_INFO_SERVER = {
	"servers",
	sizeof(struct indexed_server),
	JSON_ARRAY_SIZE +
	JSON_STRING_SIZE(IP_STRSIZE) +         /* Server IP */
	JSON_STRING_SIZE(PORT_STRSIZE) +       /* Server port */
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

int create_index(
	struct index *index, const struct index_data_info *infos,
	int (*filter)(const void *))
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
	qsort(index->data, index_ndata(index), index->infos->size, compar);
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

	/* Header */
	json_write_object_start(&jfile, NULL);
	json_write_unsigned(&jfile, "nentries", index_ndata(index));
	json_write_array_start(&jfile, "entries", index->infos->entry_size);

	if (json_have_error(&jfile))
		goto fail;

	/* Entries */
	for (i = 0; i < index_ndata(index); i++) {
		json_write_array_start(&jfile, NULL, 0);
		index->infos->write_data(&jfile, get(index, i));
		json_write_array_end(&jfile);

		if (json_have_error(&jfile))
			goto fail;

	}

	/* Footer */
	json_write_array_end(&jfile);
	json_write_object_end(&jfile);

	if (json_have_error(&jfile))
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

static int select_page(
	struct index_page *ipage, unsigned pnum, unsigned plen)
{
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
		return PAGE_NOT_FOUND;
	}

	if (!json_seek_array(&ipage->jfile, (pnum - 1) * plen))
		return PAGE_ERROR;

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

	ipage->pnum = pnum;
	ipage->i = 0;

	return PAGE_FOUND;
}

int open_index_page(
	const char *filename,
	struct index_page *ipage, const struct index_data_info *infos,
	unsigned pnum, unsigned plen)
{
	assert(ipage != NULL);
	assert(infos != NULL);
	assert((plen == 0 && pnum == 1) || (plen > 0 && pnum > 0));

	ipage->file = NULL;
	ipage->infos = infos;

	if (!dbpath(ipage->path, sizeof(ipage->path), "%s", filename))
		goto fail;

	if (!(ipage->file = fopen(ipage->path, "r"))) {
		perror(ipage->path);
		goto fail;
	}

	json_init(&ipage->jfile, ipage->file, ipage->path);

	/*
	 * First, skip header up to the array so that seeking the file
	 * use the start of the array to begin with.
	 */
	json_read_object_start(&ipage->jfile, NULL);
	json_read_unsigned(&ipage->jfile, "nentries", &ipage->ndata);
	json_read_array_start(&ipage->jfile, "entries", infos->entry_size);

	if (json_have_error(&ipage->jfile))
		goto fail;

	switch (select_page(ipage, pnum, plen)) {
	case PAGE_NOT_FOUND:
		goto not_found;
	case PAGE_ERROR:
		goto fail;
	}

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

	json_read_array_start(&ipage->jfile, NULL, 0);
	ipage->infos->read_data(&ipage->jfile, data);
	json_read_array_end(&ipage->jfile);

	if (json_have_error(&ipage->jfile))
		return 0;

	return (ipage->pnum - 1) * ipage->plen + ipage->i;
}

int index_page_dump_all(struct index_page *ipage)
{
	/*
	 * We need to avoid printing any spaces except in strings.
	 * Hence we need to check string boundaries and handle escaped
	 * characters.
	 */
	int instring = 0, forcenext = 0;
	size_t i;

	/*
	 * Since all entries have the same size we know in advance how
	 * many characters we shall read.  "entry_size" only contains
	 * the size of the entry itself, but does not count the
	 * separating comas between fields.
	 */
	const size_t pagesize =
		ipage->plen * JSON_FIELD_SIZE(ipage->infos->entry_size);

	for (i = 0; i < pagesize; i++) {
		int c = fgetc(ipage->file);

		if (c == EOF)
			return 0;

		if (forcenext)
			forcenext = 0;
		else if (instring && c == '\\')
			forcenext = 1;
		else if (!instring && c == '\"')
			instring = 1;
		else if (instring && c == '\"')
			instring = 0;
		else if (c == ' ')
			continue;

		putchar(c);
	}

	/* Make sure we are not in a middle of a json object */
	return !instring && !forcenext;
}

void close_index_page(struct index_page *ipage)
{
	assert(ipage != NULL);
	assert(ipage->file != NULL);

	fclose(ipage->file);
}
