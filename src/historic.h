#ifndef HISTORIC_H
#define HISTORIC_H

/**
 * @file historic.h
 *
 * Define data structure and functions to safely handle any data over time.
 *
 * An historic store records over time and can be read from and written to
 * a file.  An historic associate with each record user supplied data.
 *
 * Here is how to create an history:
 *
 *	hist = HISTORIC_ZERO;
 *	init_history(&hist, sizeof(player->elo), &player->elo);
 *
 * Here is how to append a record to a file:
 *
 *	hist = HISTORIC_ZERO;
 * 	init_historic(&hist, sizeof(player->elo), &player->elo);
 *
 * 	read_historic(&hist, file, path, read_elo, 0);
 * 	append_record(&hist, &new_elo);
 * 	write_historic(&hist, file, path, write_elo);
 *
 * Note that the previous historic can be re-initialized without setting it
 * to HISTORIC_ZERO.  However, it *must* be re-intialized each time.
 */

#include <stdio.h>
#include <time.h>

/**
 * @struct record
 *
 * To each record is associated data, wich can be retrieved using
 * get_record_data().  The record hold by itself the timestamp at wich
 * the associated data was recorded.  However data are stored in an
 * other buffer, hence get_record_data() to retrieve it.
 */
struct record {
	time_t time;
};

/**
 * @struct historic
 *
 * Hold historic records, any of it's field should not be modified by hand.
 */
struct historic {
	time_t epoch;
	int only_last;
	unsigned iter;

	unsigned nrecords;
	unsigned length;
	struct record last_record;
	struct record *records;

	size_t data_size;
	void *last_data;
	void *data;
};

/**
 * @def HISTORIC_ZERO
 *
 * Any fresh new historic must be set to this before a call to init_historic().
 */
const struct historic HISTORIC_ZERO;

typedef int (*read_data_func_t)(FILE *, const char *, void *);
typedef int (*write_data_func_t)(FILE *, const char *, void *);

/**
 * Initialize an historic.
 *
 * If the historic is initialized for te first time, it *must* have been
 * set with HISTORIC_ZERO before any calls to this function.
 *
 * On the other hand an history that have already been initialized do not
 * need to be set with HISTORIC_ZERO as allocated buffer can be reused.
 *
 * @param data_size Data size in historic, used for buffer allocation
 * @param last_data A special valid buffer allocated by the caller to hold
 *        last data in historic.  Hence its size must be >= than data_size
 */
void init_historic(struct historic *hist, size_t data_size, void *last_data);

/**
 * Fill an historic from the content of a file.
 *
 * init_history() *must* have been previously applied to the historic iff it is
 * the first time this function is applied to this history.
 *
 * It can reuse allocated buffers from a previous call to read_historic().
 *
 * If only_last is true, then read_data must just skip input without storing
 * it.  The provided buffer will be NULL, hence any access will result in a
 * segfault.
 *
 * @param hist Historic to be filled
 * @param file Opened file to be read
 * @param path Used as a prefix for error message
 * @param read_data Function called each time a data should be read.  It must
 *        handle it's own errors and the result should be placed into data.
 * @param only_last Only read the last record.  History will not be suitable
 *        for a call to append_record() or write_record().  In this case,
 *        read_data must skip input without storing it.
 *
 * @return 1 on success, 0 on failure
 */
int read_historic(struct historic *hist, FILE *file, const char *path,
                  read_data_func_t read_data, int only_last);

/**
 * Write the given historic to the given file.
 *
 * On success, the historic can be read back with read_data().
 *
 * @param hist Historic to be written
 * @param file File to be written
 * @param path Used as a prefix for error messages
 * @param write_data Function called each time a data should be written.
 *
 * @return 1 on success, 0 on failure
 */
int write_historic(struct historic *hist, FILE *file, const char *path,
                   write_data_func_t write_data);

/**
 * Add a record at the end of the given historic
 *
 * Supplied data are copied back to the buffer pointed by hist->last_data.
 * Failure can happen because we may realloc() data buffer and record buffer
 * if they are too small to contain the new record.
 *
 * @param hist Historic where the data must be added
 * @param data Data to be recorded
 *
 * @return 1 on success, 0 on failure
 */
int append_record(struct historic *hist, const void *data);

/**
 * Return first record in historic
 *
 * @param hist Historic to get the first record from
 *
 * @return First record, NULL if any
 */
struct record *first_record(struct historic *hist);

/**
 * Return last record in historic
 *
 * @param hist Historic to get the last record from
 *
 * @return Last record, NULL if any
 */
struct record *last_record(struct historic *hist);

/**
 * Iterate over records, return NULL when no records are available.
 *
 * Once NULL is returned, another loop start and the first record is returned.
 * Hence nesting calls to next_record() for the same historic doesn't works.
 *
 * @param hist Historic to be iterated upon
 */
struct record *next_record(struct historic *hist);

/**
 * Compute record index as if records were in an array from the oldest
 * record the the newest one.
 *
 * @param hist Historic to search record index
 * @param record Record to get the index for
 *
 * @return Index of the given record
 */
unsigned record_index(struct historic *hist, struct record *record);

/**
 * Return the number of records
 *
 * @param hist History to query the number of records
 *
 * @return Number fo records
 */
unsigned get_nrecords(struct historic *hist);

/**
 * Given a record, return associated data.
 *
 * @param hist History the given record belongs to
 * @param record The record to get the associated data
 *
 * @return Pointer to data, it will always return a valid pointer
 */
void *get_record_data(struct historic *hist, struct record *record);

#endif /* HISTORIC_H */
