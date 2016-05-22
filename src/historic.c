#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "historic.h"

void init_historic(struct historic *hist, size_t data_size, void *last_data)
{
	assert(hist != NULL);
	assert(data_size > 0);
	assert(last_data != NULL);

	hist->data_size = data_size;
	hist->last_data = last_data;

	hist->nrecords = 0;
	hist->only_last = 0;
	hist->iter = 0;
}

static int read_record(FILE *file, const char *path, struct historic *hist, struct record *record,
                       read_data_func_t read_data)
{
	int ret;

	errno = 0;
	ret = fscanf(file, " %lu", &record->time);
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	else if (ret == EOF && ret == 0)
		return fprintf(stderr, "%s: Cannot match record time\n", path), 0;

	record->time += hist->epoch;

	if (!read_data(file, path, get_record_data(hist, record)))
		return 0;

	return 1;
}

static int skip_record(FILE *file, const char *path, read_data_func_t read_data)
{
	int ret;

	errno = 0;
	ret = fscanf(file, " %*u");
	if (ret == EOF && errno != 0)
		return perror(path), 0;
	if (!read_data(file, path, NULL))
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

	if (!hist->only_last && nrecords > hist->length) {
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

static int read_historic_header(struct historic *hist, FILE *file, const char *path)
{
	int ret;
	unsigned nrecords;

	errno = 0;
	ret = fscanf(file, " %u records starting at %lu\n", &nrecords, &hist->epoch);
	if (ret == EOF && errno != 0)
		return fprintf(stderr, "%s: %s\n", path, strerror(errno)), 0;
	else if (ret == EOF || ret == 0)
		return fprintf(stderr, "%s: Cannot match number of record\n", path), 0;
	else if (ret == 1)
		return fprintf(stderr, "%s: Cannot match epoch\n", path), 0;

	if (!set_nrecords(hist, nrecords))
		return 0;

	return 1;
}

int read_historic(struct historic *hist, FILE *file, const char *path,
                  read_data_func_t read_data, int only_last)
{
	unsigned i;

	assert(hist != NULL);
	assert(file != NULL);
	assert(path != NULL);
	assert(read_data != NULL);

	/* Set it first so following calls have the desired effect */
	hist->only_last = only_last;

	if (!read_historic_header(hist, file, path))
		return 0;

	if (hist->nrecords == 0)
		return 1;

	if (only_last) {
		if (read_record(file, path, hist, last_record(hist), read_data) == 0)
			return 0;

		for (i = 1; i < hist->nrecords; i++)
			if (!skip_record(file, path, read_data))
				return 0;
	} else {
		for (i = 0; i < hist->nrecords; i++) {
			struct record *record = &hist->records[hist->nrecords - i - 1];

			if (i != 0)
				fscanf(file, " ,");
			if (read_record(file, path, hist, record, read_data) == 0)
				return 0;
		}
	}

	return 1;
}

static void write_record(FILE *file, const char *path, struct historic *hist, struct record *record,
                         write_data_func_t write_data)
{
	fprintf(file, "%lu ", record->time - hist->epoch);
	write_data(file, path, get_record_data(hist, record));
}

int write_historic(struct historic *hist, FILE *file, const char *path,
                   write_data_func_t write_data)
{
	unsigned i;

	assert(hist != NULL);
	assert(file != NULL);
	assert(path != NULL);
	assert(write_data != NULL);
	assert(!hist->only_last);

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

unsigned record_index(struct historic *hist, struct record *record)
{
	assert(hist != NULL);
	assert(record != NULL);
	assert(!hist->only_last);

	return record - hist->records;
}

struct record *last_record(struct historic *hist)
{
	if (hist->nrecords == 0)
		return NULL;
	else if (hist->only_last)
		return &hist->last_record;
	else
		return &hist->records[hist->nrecords - 1];
}

struct record *first_record(struct historic *hist)
{
	assert(!hist->only_last);

	if (hist->nrecords == 0)
		return NULL;
	else
		return &hist->records[0];
}

void *get_record_data(struct historic *hist, struct record *record)
{
	if (record == last_record(hist))
		return hist->last_data;
	else
		return &((char*)hist->data)[record_index(hist, record) * hist->data_size];
}

static struct record *previous_record(struct historic *hist, struct record *record)
{
	if (record == hist->records)
		return NULL;
	return record - 1;
}

int append_record(struct historic *hist, const void *data)
{
	struct record *prev, *last;
	time_t now = time(NULL);

	assert(hist != NULL);
	assert(!hist->only_last);

	/* Create history if empty */
	if (hist->nrecords == 0)
		hist->epoch = now;

	if (!set_nrecords(hist, hist->nrecords + 1))
		return 0;

	/* Set last record with new values */

	last = last_record(hist);
	assert(last != NULL);

	/* Archive previous last record */
	prev = previous_record(hist, last);
	if (prev) {
		memcpy(get_record_data(hist, prev), hist->last_data, hist->data_size);
	}

	last->time = now;
	memcpy(get_record_data(hist, last), data, hist->data_size);

	return 1;
}

struct record *next_record(struct historic *hist)
{
	assert(hist != NULL);
	assert(!hist->only_last);

	if (hist->iter == hist->nrecords) {
		hist->iter = 0;
		return NULL;
	} else {
		return &hist->records[hist->iter++];
	}
}

unsigned get_nrecords(struct historic *hist)
{
	return hist->nrecords;
}
