#ifndef INDEX_H
#define INDEX_H

#include <limits.h>

#include "player.h"
#include "server.h"

/*
 * Index provide an easy way to load all files from a directory, sort
 * them and write an index to quickly access part of the sorted list.
 *
 * To make sorting data faster, it only store data used for sorting in
 * the index.  Hence create index take an already parametred
 * index_type, and with this particular type comes a structure used
 * to iterate through sorted data if needed.
 *
 * For instance, sorting player by rank only requitre player's name and
 * their rank.  Hence struct player_name_and_rank.
 *
 * You can retrieve part of the sorted list using index_page API.
 * Index info feeded to
 */

extern const struct index_data_infos *INDEX_DATA_INFOS_PLAYER;
struct indexed_player {
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];
	int elo;
	unsigned rank;
	struct tm last_seen;
};

extern const struct index_data_infos *INDEX_DATA_INFOS_CLAN;
struct indexed_clan {
	char name[HEXNAME_LENGTH];
	unsigned nmembers;
};

extern const struct index_data_infos *INDEX_DATA_INFOS_SERVER;
struct indexed_server {
	char name[SERVERNAME_STRSIZE];
	char gametype[GAMETYPE_STRSIZE];
	char map[MAP_STRSIZE];
	unsigned nplayers;
	unsigned maxplayers;
};

struct index {
	const struct index_data_infos *infos;

	unsigned ndata, bufsize;
	void *data;

	int reuse_last;
	unsigned i;
};

/**
 * Create an index for the given data
 *
 * @param index Index to create
 * @param infos Infos about data the index should contain
 *
 * @return 1 on success, 0 on failure
 */
int create_index(struct index *index, const struct index_data_infos *infos);

/**
 * Sort index entry using the given function
 *
 * @param index Index to sort
 * @param compar Comparison function used for qsort()
 */
void sort_index(struct index *index, int (*compar)(const void *, const void *));

/**
 * Loop through index entries
 *
 * @param index Index to loop on
 *
 * @return Next index entry if any, NULL otherwise
 */
void *index_foreach(struct index *index);

/**
 * Write the index to the given file
 *
 * Index can then be loaded using open_index_page().
 *
 * @param index Index to write
 * @param filename Filename of the index file in the database
 *
 * @return 1 on success, 0 on failure
 */
int write_index(struct index *index, const char *filename);

/**
 * Free memory holded by the given index
 *
 * @param index Index to close
 */
void close_index(struct index *index);

struct index_page {
	const struct index_data_infos *infos;

	void *databuf;
	char *tmpname;

	FILE *file;
	char path[PATH_MAX];

	unsigned ndata, npages;
	unsigned pnum, plen, i;
};

enum {
	PAGE_FOUND,
	PAGE_NOT_FOUND,
	PAGE_ERROR,
};

/**
 * Load a page of an index
 *
 * If page length is 0, then the whole index is loaded and page number
 * should be 1.
 *
 * @param filename Filename of the index file in the database
 * @param ipage Index page
 * @param infos Index data infos
 * @param pnum Page number, starts from 1
 * @param plen Pages length
 *
 * @return PAGE_FOUND when page does exist, PAGE_NOT_FOUND when it
 *         doesn't exist, PAGE_ERROR otherwise.
 */
int open_index_page(
	const char *filename,
	struct index_page *ipage, const struct index_data_infos *infos,
	unsigned pnum, unsigned plen);

/**
 * Loop through entries in the given index page
 *
 * On success, data will contain the content of the next index entry.
 * However, on failure data should not be accessed and will contain
 * random values.
 *
 * @param ipage Index page to loop on
 * @param data A valid buffer to store the next index entry
 *
 * @return 0 if every entries in the page has been iterated through,
 *         an absolute entry number otherwise (a positive number)
 */
unsigned index_page_foreach(struct index_page *ipage, void *data);

/**
 * Close the given index page
 *
 * Free memory and opened file.
 *
 * @param ipage Index page to close
 */
void close_index_page(struct index_page *ipage);

#endif /* INDEX_H */