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
 *	init_historic(&hist, sizeof(player->elo), UINT_MAX, 0);
 *
 * Here is how to append a record to a file:
 *
 *	hist = HISTORIC_ZERO;
 * 	init_historic(&hist, sizeof(player->elo), UINT_MAX, 0);
 *
 * 	read_historic(&hist, file, path, read_elo);
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
 * record_data().  The record hold by itself the timestamp at wich
 * the associated data was recorded.  However data are stored in an
 * other buffer, hence record_data() to retrieve it.
 */
struct record {
	time_t time;
	struct record *prev, *next;
};

/**
 * @struct historic
 *
 * Hold full historic records
 */
struct historic {
	time_t epoch;
	time_t timestep;

	unsigned nrecords;
	unsigned max_records;
	unsigned length;
	struct record *records;

	struct record *first, *last;

	size_t data_size;
	void *data;
};

/**
 * @def HISTORIC_ZERO
 *
 * Any fresh new historic must be set to this before a call to init_historic().
 */
const struct historic HISTORIC_ZERO;

typedef int (*read_data_func_t)(FILE *, const char *, void *);
typedef int (*skip_data_func_t)(FILE *, const char *);
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
 * Timestep is the minimum time gap between two records.  Except for
 * the very last record, which can have any timestamp without any
 * constraints.
 *
 * Use UINT_MAX from limits.h if you want no limits on the maximum
 * number of records.
 *
 * @param hist Historic to initialize
 * @param data_size Data size in historic, used for buffer allocation
 * @param max_records Maximum number of records
 * @param timestep Minimum time gap between two records
 */
void init_historic(struct historic *hist, size_t data_size,
                   unsigned max_records, time_t timestep);

/**
 * TODO
 */
void create_historic(struct historic *hist);

/**
 * Fill an historic from the content of a file.
 *
 * init_history() *must* have been previously applied to the historic iff it is
 * the first time this function is applied to this history.
 *
 * It can reuse allocated buffers from a previous call to read_historic().
 *
 * @param hist Historic to be filled
 * @param file Opened file to be read
 * @param path Used as a prefix for error message
 * @param read_data Function called each time a data should be read.  It must
 *        handle it's own errors and the result should be placed into data.
 *
 * @return 1 on success, 0 on failure
 */
int read_historic(struct historic *hist, FILE *file, const char *path,
                  read_data_func_t read_data);

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
 * Return a pointer to the associated data of the given record.
 *
 * @param hist Historic the given record belongs to
 * @param record record to get the associated data
 *
 * @return Pointer to data
 */
void *record_data(struct historic *hist, struct record *record);

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
 * @struct historic_summary
 *
 * Summarize an historic by just storing the last record.
 */
struct historic_summary {
	time_t epoch;
	unsigned nrecords;
	struct record last_record;
};

/**
 * Fill the given historic summary with the content of the given file
 *
 * @param hs Historic summary to fill
 * @param file File to read the historic summary from
 * @param path Used as a prefix for error message
 * @param read_data Function called to fill paramater last_data
 * @param skip_data Function called to skip unused historic data
 * @param last_data a pointer to a valid buaffer to hold the last data
 *
 * @return 1 on success, 0 on failure
 */
int read_historic_summary(struct historic_summary *hs, FILE *file, const char *path,
                          read_data_func_t read_data, skip_data_func_t skip_data, void *last_data);

#endif /* HISTORIC_H */
