#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "historic.h"
#include "json.h"

void init_historic(struct historic *hist, size_t data_size,
                   unsigned max_records)
{
	static const struct historic HISTORIC_ZERO;

	assert(hist != NULL);
	assert(data_size > 0);
	assert(max_records > 0);

	*hist = HISTORIC_ZERO;
	hist->data_size = data_size;
	hist->max_records = max_records;
}

/*
 * This function try to predict how much record will be added to an
 * existing historic.  It also make sure we don't realloc() buffer
 * too often when historics are re-used.
 */
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

static int read_record(
	struct jfile *jfile, time_t epoch, struct record *record, void *data,
	read_data_func_t read_data)
{
	unsigned delta;

	assert(data != NULL);

	json_read_array_start(jfile, NULL, 0);
	json_read_unsigned(jfile, NULL, &delta);
	read_data(jfile, data);
	json_read_array_end(jfile);

	if (json_have_error(jfile))
		return 0;

	record->time = epoch + delta;

	return 1;
}

/* Never fail */
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

int read_historic(
	struct historic *hist, struct jfile *jfile,
	read_data_func_t read_data)
{
	unsigned i;

	assert(hist != NULL);
	assert(jfile != NULL);
	assert(read_data != NULL);

	reset_historic(hist);

	json_read_unsigned(   jfile, "length", &hist->nrecords);
	json_read_time(       jfile, "epoch" , &hist->epoch);
	json_read_array_start(jfile, "records", 0);

	if (json_have_error(jfile))
		return 0;

	if (hist->nrecords == 0)
		return json_print_error(jfile, "Empty historic forbidden");

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

		if (read_record(jfile, hist->epoch, rec, data, read_data) == 0)
			return 0;
	}

	if (!json_read_array_end(jfile))
		return 0;

	hist->first = &hist->records[0];
	hist->last = &hist->records[hist->nrecords - 1];
	hist->last->next = NULL;
	hist->first->prev = NULL;

	return 1;
}

static int write_record(
	struct jfile *jfile, struct historic *hist, struct record *record,
	write_data_func_t write_data)
{
	json_write_array_start(jfile, NULL, 0);

	json_write_unsigned(jfile, NULL, record->time - hist->epoch);
	write_data(jfile, record_data(hist, record));

	json_write_array_end(jfile);

	return !json_have_error(jfile);
}

int write_historic(
	struct historic *hist, struct jfile *jfile,
	write_data_func_t write_data)
{
	struct record *rec;

	assert(hist != NULL);
	assert(jfile != NULL);
	assert(write_data != NULL);

	json_write_unsigned(jfile, "length", hist->nrecords);
	json_write_time(jfile, "epoch", hist->epoch);
	json_write_array_start(jfile, "records", 0);

	if (json_have_error(jfile))
		return 0;

	for (rec = hist->last; rec; rec = rec->prev)
		if (!write_record(jfile, hist, rec, write_data))
			return 0;

	json_write_array_end(jfile);

	return !json_have_error(jfile);
}

void *record_data(struct historic *hist, struct record *record)
{
	return (char*)hist->data + (record - hist->records) * hist->data_size;
}

void append_record(struct historic *hist, const void *data)
{
	struct record *rec;
	time_t now = time(NULL);

	assert(hist != NULL);

	rec = new_record(hist);
	rec->time = now;
	memcpy(record_data(hist, rec), data, hist->data_size);
}

void read_historic_info(struct historic_info *hs, struct jfile *jfile)
{
	json_read_unsigned(jfile, "length", &hs->nrecords);
	json_read_time(    jfile, "epoch" , &hs->epoch);
}
