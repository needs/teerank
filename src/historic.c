#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "historic.h"

void init_historic(struct historic *hist, size_t data_size,
                   unsigned max_records, time_t timestep)
{
	assert(hist != NULL);
	assert(data_size > 0);
	assert(max_records > 0);

	hist->data_size = data_size;
	hist->timestep = timestep;
	hist->max_records = max_records;
}

static unsigned round_length(unsigned length)
{
	const unsigned LENGTH_STEP = 1024;
	const unsigned EXTRA_LENGTH = 5;
	length += EXTRA_LENGTH;

	return length - (length % LENGTH_STEP) + LENGTH_STEP;
}

/*
 * Does malloc() buffers to fit the required length.  If pre-existing
 * buffers are wide enough, it just use them.  If not, it free() them
 * and malloc() them again.  We are not using realloc() to avoid
 * copying data in the new buffer.
 */
static int alloc_historic(struct historic *hist, unsigned length)
{
	assert(hist != NULL);

	length = round_length(length);
	assert(length);

	if (length > hist->length) {

		if (hist->records)
			free(hist->records);
		if (hist->data)
			free(hist->data);

		/* TODO: Call malloc() only once */

		/* Grow records buffer */
		hist->records = malloc(length * sizeof(*hist->records));
		if (!hist->records)
			return 0;

		/* Grow data buffer */
		hist->data = malloc(length * hist->data_size);
		if (!hist->data)
			return 0;

		hist->length = length;
	}

	return 1;
}

static void reset_historic(struct historic *hist)
{
	hist->first = NULL;
	hist->last = NULL;
	hist->nrecords = 0;
}

void create_historic(struct historic *hist)
{
	reset_historic(hist);

	hist->epoch = time(NULL);
	alloc_historic(hist, 0);
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

static struct record *new_record(struct historic *hist)
{
	assert(hist != NULL);
	assert(hist->max_records > 0);
	assert(hist->nrecords <= hist->max_records);

	if (hist->nrecords == hist->max_records) {
		/* Recycle first record as the last one */
		hist->last->next = hist->first;
		hist->first->prev = hist->last;
		hist->first = hist->first->next;
		hist->last = hist->last->next;
	} else {
		struct record *rec;

		assert(hist->nrecords < hist->length);

		rec = &hist->records[hist->nrecords];
		hist->nrecords++;

		if (!hist->first) {
			hist->first = rec;
			hist->last = rec;
		} else {
			hist->last->next = rec;
			rec->prev = hist->last;
			hist->last = rec;
		}
	}

	/* Those invariants should always hold */
	hist->first->prev = NULL;
	hist->last->next = NULL;

	return hist->last;
}

int read_historic(struct historic *hist, FILE *file, const char *path,
                  read_data_func_t read_data)
{
	unsigned i;

	assert(hist != NULL);
	assert(file != NULL);
	assert(path != NULL);
	assert(read_data != NULL);

	reset_historic(hist);

	if (!read_historic_header(file, path, &hist->nrecords, &hist->epoch))
		return 0;

	if (hist->nrecords == 0) {
		fprintf(stderr, "%s: Empty historic forbidden\n", path);
		return 0;
	}

	if (!alloc_historic(hist, hist->nrecords))
		return 0;

	for (i = 0; i < hist->nrecords; i++) {
		struct record *rec;
		void *data;

		/* Records are written in reverse order... */
		rec = &hist->records[hist->nrecords - i - 1];
		data = record_data(hist, rec);

		/* Link record with the previous one and next one */
		if (i > 0)
			rec->next = rec + 1;
		if (i < hist->nrecords - 1)
			rec->prev = rec - 1;

		if (i != 0)
			fscanf(file, " ,");
		if (read_record(file, path, hist->epoch, rec, data, read_data) == 0)
			return 0;
	}

	hist->first = &hist->records[0];
	hist->last = &hist->records[hist->nrecords - 1];
	hist->last->next = NULL;
	hist->first->prev = NULL;

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
	struct record *rec;

	assert(hist != NULL);
	assert(file != NULL);
	assert(path != NULL);
	assert(write_data != NULL);

	fprintf(file, "%u records starting at %lu\n", hist->nrecords, hist->epoch);

	for (rec = hist->last; rec; rec = rec->prev) {
		if (rec != hist->last)
			fprintf(file, ", ");

		write_record(file, path, hist, rec, write_data);
	}

	fputc('\n', file);

	return 1;
}

void *record_data(struct historic *hist, struct record *record)
{
	return (char*)hist->data + (record - hist->records) * hist->data_size;
}

static int same_timeframe(time_t a, time_t b, time_t timestep)
{
	if (timestep)
		return a / timestep == b / timestep;
	else
		return a == b;
}

static time_t next_timeframe(time_t t, time_t timestep)
{
	if (timestep)
		return (t - (t % timestep)) + timestep;
	else
		return t;
}

int append_record(struct historic *hist, const void *data)
{
	struct record *last;
	time_t now = time(NULL);

	assert(hist != NULL);

	last = hist->last;

	if (!last) {
		last = new_record(hist);
	} else if (!same_timeframe(last->time, now, hist->timestep)) {
		if (hist->timestep)
			printf("last = %lu, now = %lu\n",
			       (last->time - hist->epoch) / hist->timestep,
			       (now - hist->epoch) / hist->timestep);

		last->time = next_timeframe(last->time, hist->timestep);
		last = new_record(hist);
	}

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
