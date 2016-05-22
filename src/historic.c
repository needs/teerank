#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "historic.h"

void init_historic(struct historic *hist, size_t data_size)
{
	assert(hist != NULL);
	assert(data_size > 0);

	hist->data_size = data_size;

	hist->nrecords = 0;
}

static int read_record(FILE *file, const char *path, time_t epoch, struct record *record, void *data,
                       read_data_func_t read_data)
{
	int ret;

	assert(data != NULL);

	errno = 0;
	ret = fscanf(file, " %lu", &record->time);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF && ret == 0)
		return fprintf(stderr, "%s: Cannot match record time\n", path), 0;

	record->time += epoch;

	if (!read_data(file, path, data))
		return 0;

	return 1;
}

static int skip_record(FILE *file, const char *path, skip_data_func_t skip_data)
{
	int ret;

	errno = 0;
	ret = fscanf(file, " %*u");
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	if (skip_data(file, path) == 0)
		return 0;

	return 1;
}

static unsigned get_length(unsigned nrecords)
{
	const unsigned LENGTH_STEP = 1024;
	const unsigned LENGTH_EXTRA = 1;
	unsigned length = nrecords + LENGTH_EXTRA;

	return length - (length % LENGTH_STEP) + LENGTH_STEP;
}

/* Does realloc() buffers if they are too small */
static int set_nrecords(struct historic *hist, unsigned nrecords)
{
	void *tmp;

	assert(hist != NULL);

	if (nrecords > hist->length) {
		const unsigned length = get_length(nrecords);

		/* Grow records buffer */
		tmp = realloc(hist->records, length * sizeof(*hist->records));
		if (!tmp)
			return 0;
		hist->records = tmp;

		/* Grow data buffer */
		tmp = realloc(hist->data, length * hist->data_size);
		if (!tmp)
			return 0;
		hist->data = tmp;

		hist->length = length;
	}

	hist->nrecords = nrecords;
	return 1;
}

static int read_historic_header(FILE *file, const char *path,
                                unsigned *nrecords, time_t *epoch)
{
	int ret;

	assert(nrecords != NULL);
	assert(epoch != NULL);

	errno = 0;
	ret = fscanf(file, " %u records starting at %lu\n", nrecords, epoch);
	if (ret == EOF && errno != 0)
		return fprintf(stderr, "%s: %s\n", path, strerror(errno)), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match number of record\n", path), 0;
	else if (ret == 1)
		return fprintf(stderr, "%s: Cannot match epoch\n", path), 0;

	return 1;
}

int read_historic(struct historic *hist, FILE *file, const char *path,
                  read_data_func_t read_data)
{
	unsigned i;

	assert(hist != NULL);
	assert(file != NULL);
	assert(path != NULL);
	assert(read_data != NULL);

	if (!read_historic_header(file, path, &hist->nrecords, &hist->epoch))
		return 0;

	if (hist->nrecords == 0)
		return 1;

	if (!set_nrecords(hist, hist->nrecords))
		return 0;

	for (i = 0; i < hist->nrecords; i++) {
		struct record *record = &hist->records[hist->nrecords - i - 1];
		void *data = record_data(hist, record);

		if (i != 0)
			fscanf(file, " ,");
		if (read_record(file, path, hist->epoch, record, data, read_data) == 0)
			return 0;
	}

	return 1;
}

static void write_record(FILE *file, const char *path, struct historic *hist, struct record *record,
                         write_data_func_t write_data)
{
	fprintf(file, "%lu ", record->time - hist->epoch);
	write_data(file, path, record_data(hist, record));
}

int write_historic(struct historic *hist, FILE *file, const char *path,
                   write_data_func_t write_data)
{
	unsigned i;

	assert(hist != NULL);
	assert(file != NULL);
	assert(path != NULL);
	assert(write_data != NULL);

	fprintf(file, "%u records starting at %lu\n", hist->nrecords, hist->epoch);

	for (i = 0; i < hist->nrecords; i++) {
		struct record *record = &hist->records[hist->nrecords - i - 1];

		if (i != 0)
			fprintf(file, ", ");

		write_record(file, path, hist, record, write_data);
	}

	fputc('\n', file);

	return 1;
}

struct record *last_record(struct historic *hist)
{
	if (hist->nrecords == 0)
		return NULL;
	else
		return &hist->records[hist->nrecords - 1];
}

struct record *first_record(struct historic *hist)
{
	if (hist->nrecords == 0)
		return NULL;
	else
		return &hist->records[0];
}

void *record_data(struct historic *hist, struct record *record)
{
	return &((char*)hist->data)[(record - hist->records) * hist->data_size];
}

int append_record(struct historic *hist, const void *data)
{
	struct record *last;
	time_t now = time(NULL);

	assert(hist != NULL);

	/* Create history if empty */
	if (hist->nrecords == 0)
		hist->epoch = now;

	if (!set_nrecords(hist, hist->nrecords + 1))
		return 0;

	/* Set last record with new values */

	last = last_record(hist);
	assert(last != NULL);

	last->time = now;
	memcpy(record_data(hist, last), data, hist->data_size);

	return 1;
}

int read_historic_summary(struct historic_summary *hs, FILE *file, const char *path,
                          read_data_func_t read_data, skip_data_func_t skip_data, void *last_data)
{
	unsigned i;

	if (read_historic_header(file, path, &hs->nrecords, &hs->epoch) == 0)
		return 0;

        if (hs->nrecords == 0)
	        return 1;

        /* Last record */
        if (read_record(file, path, hs->epoch, &hs->last_record, last_data, read_data) == 0)
	        return 0;

        /* Skip remaning records */
        for (i = 1; i < hs->nrecords; i++) {
	        fscanf(file, ", ");
	        if (skip_record(file, path, skip_data) == 0)
		        return 0;
        }

        return 1;
}
