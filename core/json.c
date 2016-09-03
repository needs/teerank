#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "json.h"

void json_init(struct jfile *jfile, FILE *file, const char *path)
{
	static const struct jfile JFILE_ZERO;

	*jfile = JFILE_ZERO;
	jfile->file = file;
	jfile->path = path;
	jfile->scope = &jfile->scopes[0];
}

#define error(jfile, fmt...) json_print_error(jfile, fmt)

int json_print_error(struct jfile *jfile, const char *fmt, ...)
{
	va_list ap;

	jfile->have_error = 1;

	/* Suppose file is a oneliner */
	fprintf(stderr, "%s:1:%ld: ", jfile->path, ftell(jfile->file));

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
	return 0;
}

int json_have_error(struct jfile *jfile)
{
	return jfile->have_error;
}

static char skip_space(struct jfile *jfile)
{
	int c;

	do
		c = fgetc(jfile->file);
	while (c != EOF && isspace(c));

	return c;
}

static int read_field_name(struct jfile *jfile, const char *fname)
{
	const char *fname_save = fname;
	int c;

	assert(jfile->scope);

	if (json_have_error(jfile))
		return 0;

	/* First should not be preceeded by a ','. */
	if (jfile->scope->field_count > 0 && skip_space(jfile) != ',') {
		if (fname)
			return error(jfile, "Expected ',' before %s", fname);
		else
			return error(jfile, "Expected ','");
	}

	jfile->scope->field_count++;

	if (!fname)
		return 1;

	/*
	 * JSON field name looks like a string.  They are enclosed with
	 * double quotes and a ':' must follow.
	 */

	if (skip_space(jfile) != '\"')
		return error(jfile, "Expected '\"' to start field name");

	/* Compare the read field name with the given one as we go. */
	goto start;
	while (*fname) {
		if (c == '\"')
			break;
		if (c != *fname)
			return error(jfile, "Expected field name \"%s\"", fname_save);
		fname++;
	start:
		c = fgetc(jfile->file);
	}

	if (*fname != '\0')
		return error(jfile, "Expected '\"' to end field name");
	if (fgetc(jfile->file) != ':')
		return error(jfile, "Expected ':' after field name");

	return 1;
}

/*
 * Set scope pointer to the new scope, reset field count save the
 * current file position so we can compute the size of the object once
 * we close the scope.
 */
static int increase_scope_level(struct jfile *jfile)
{
	assert(jfile->scope_level + 1 < MAX_SCOPE_LEVEL);

	jfile->scope_level++;
	jfile->scope = &jfile->scopes[jfile->scope_level];

	jfile->scope->field_count = 0;
	jfile->scope->offset = ftello(jfile->file);

	if (jfile->scope->offset == -1)
		return error(jfile, "%s", strerror(errno));

	return 1;
}

static int fix_field_size(struct jfile *jfile, size_t field_size)
{
	size_t i, remain;

	if (jfile->scope->field_size == 0)
		return 1;

	/*
	 * Write the necessary amount of spaces to have a field with a
	 * size exactly equals to jfile->scope->field_size.
	 */

	if (field_size > jfile->scope->field_size)
		return error(jfile, "Actual field size exceeded the given maximum (%zu / %zu)",
		             field_size, jfile->scope->field_size);

	remain = jfile->scope->field_size - field_size;

	for (i = 0; i < remain; i++)
		if (fputc(' ', jfile->file) == EOF)
			return error(jfile, "%s", strerror(errno));

	return 1;
}

static size_t scope_size(struct jfile *jfile)
{
	off_t offset;

	offset = ftello(jfile->file);
	if (offset == -1)
		return error(jfile, "%s", strerror(errno));

	return offset - jfile->scope->offset;
}

/*
 * Set scope pointer to the previous scope, compute and return object
 * size.
 */
static size_t decrease_scope_level(struct jfile *jfile, int do_fix_field_size)
{
	size_t size;

	assert(jfile->scope_level > 0);

	if ((size = scope_size(jfile)) == 0)
		return 0;

	jfile->scope_level--;
	jfile->scope = &jfile->scopes[jfile->scope_level];

	if (do_fix_field_size) {
		/*
		 * Fix field size now so that every fields, including
		 * the last field will be padded.  It is important for
		 * the last field to be padded to know the total size of
		 * the array given the number of elements.
		 */
		fix_field_size(jfile, size);
	}

	return size;
}

int json_read_object_start(struct jfile *jfile, const char *fname)
{
	if (!read_field_name(jfile, fname))
		return 0;
	if (!increase_scope_level(jfile))
		return 0;
	if (skip_space(jfile) != '{')
		return error(jfile, "Expected '{' to start object");

	return 1;
}

size_t json_read_object_end(struct jfile *jfile)
{
	if (skip_space(jfile) != '}')
		return error(jfile, "Expected '}' to end object");

	return decrease_scope_level(jfile, 0);
}

/* Doesn't escape special characters */
static int write_string(struct jfile *jfile, const char *str)
{
	if (fputc('\"', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));
	if (fputs(str, jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));
	if (fputc('\"', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return 1;
}

static int write_field_name(struct jfile *jfile, const char *fname)
{
	if (json_have_error(jfile))
		return 0;

	/*
	 * The first field does not need to be preceeded by a coma.
	 * However we need to add a space if the field must have a fixed
	 * size, so that it have the same size as fields with a
	 * preceeding coma.
	 */
	if (jfile->scope->field_count > 0) {
		if (fputc(',', jfile->file) == EOF)
			return error(jfile, "%s", strerror(errno));
	} else if (jfile->scope->field_size) {
		if (fputc(' ', jfile->file) == EOF)
			return error(jfile, "%s", strerror(errno));
	}

	jfile->scope->field_count++;

	if (!fname)
		return 1;

	if (!write_string(jfile, fname))
		return 0;

	if (fputc(':', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return 1;
}

int json_write_object_start(struct jfile *jfile, const char *fname)
{
	if (!write_field_name(jfile, fname))
		return 0;
	if (!increase_scope_level(jfile))
		return 0;
	if (fputc('{', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return 1;
}

size_t json_write_object_end(struct jfile *jfile)
{
	assert(jfile != NULL);
	assert(jfile->scope_level > 0);

	if (fputc('}', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return decrease_scope_level(jfile, 1);
}

int json_read_array_start(struct jfile *jfile, const char *fname)
{
	if (!read_field_name(jfile, fname))
		return 0;
	if (!increase_scope_level(jfile))
		return 0;
	if (skip_space(jfile) != '[')
		return error(jfile, "Expected '[' to start array");

	return 1;
}

size_t json_read_array_end(struct jfile *jfile)
{
	if (skip_space(jfile) != ']')
		return error(jfile, "Expected ']' to end array");

	return decrease_scope_level(jfile, 0);
}

int json_read_indexable_array_start(struct jfile *jfile, const char *fname, size_t field_size)
{
	if (!json_read_array_start(jfile, fname))
		return 0;

	jfile->scope->field_size = field_size;

	return 1;
}

size_t json_read_indexable_array_end(struct jfile *jfile)
{
	return json_read_array_end(jfile);
}

int json_write_array_start(struct jfile *jfile, const char *fname)
{
	if (!write_field_name(jfile, fname))
		return 0;
	if (!increase_scope_level(jfile))
		return 0;
	if (fputc('[', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return 1;
}

size_t json_write_array_end(struct jfile *jfile)
{
	assert(jfile != NULL);
	assert(jfile->scope_level > 0);

	if (fputc(']', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return decrease_scope_level(jfile, 1);
}

int json_write_indexable_array_start(
	struct jfile *jfile, const char *fname, size_t field_size)
{
	if (!json_write_array_start(jfile, fname))
		return 0;

	jfile->scope->field_size = field_size;

	return 1;
}

size_t json_write_indexable_array_end(struct jfile *jfile)
{
	return json_write_array_end(jfile);
}

int json_seek_indexable_array(struct jfile *jfile, unsigned n)
{
	off_t offset;

	assert(jfile != NULL);
	assert(jfile->scope != NULL);
	assert(jfile->scope->field_size);

	/* Add 1 to count coma */
	offset = n * (jfile->scope->field_size + 1);

	if (fseek(jfile->file, offset, SEEK_CUR) == -1)
		return error(jfile, "%s", strerror(errno));

	jfile->scope->field_count = n;
	return 1;
}

int json_read_int(struct jfile *jfile, const char *fname, int *buf)
{
	int ret;

	if (!read_field_name(jfile, fname))
		return 0;

	errno = 0;
	ret = fscanf(jfile->file, "%d", buf);
	if (ret == EOF && errno != 0)
		return error(jfile, "%s", strerror(errno));
	else if (ret == EOF || ret == 0)
		return error(jfile, "Cannot match integer");

	return 1;
}

int json_write_int(struct jfile *jfile, const char *fname, int buf)
{
	int ret;

	if (!write_field_name(jfile, fname))
		return 0;

	ret = fprintf(jfile->file, "%d", buf);
	if (ret < 0)
		return error(jfile, "%s", strerror(errno));

	return ret;
}

int json_read_unsigned(struct jfile *jfile, const char *fname, unsigned *buf)
{
	int ret;

	if (!read_field_name(jfile, fname))
		return 0;

	errno = 0;
	ret = fscanf(jfile->file, "%u", buf);
	if (ret == EOF && errno != 0)
		return error(jfile, "%s", strerror(errno));
	else if (ret == EOF || ret == 0)
		return error(jfile, "Cannot match unsigned integer");

	return 1;
}

int json_write_unsigned(struct jfile *jfile, const char *fname, unsigned buf)
{
	int ret;

	if (!write_field_name(jfile, fname))
		return 0;

	ret = fprintf(jfile->file, "%u", buf);
	if (ret < 0)
		return error(jfile, "%s", strerror(errno));

	return ret;
}

int json_read_string(struct jfile *jfile, const char *fname, char *buf, size_t size)
{
	int c;

	assert(size > 0);

	if (!read_field_name(jfile, fname))
		return 0;

	if (skip_space(jfile) != '\"')
		return error(jfile, "Expected '\"' to start string");

	goto start;

	/*
	 * Size should always be greater than 1 because we left a byte
	 * to append the nul terminating character.
	 */
	while (size > 1 && c != '\"' && c != EOF) {
		*buf = c;

		buf++;
		size--;
	start:
		c = fgetc(jfile->file);
	}

	*buf = '\0';

	if (c == '\"')
		return 1;
	else if (c == EOF)
		return error(jfile, "Expected '\"' to end string");
	else
		return error(jfile, "String too long");
}

int json_write_string(struct jfile *jfile, const char *fname, const char *buf, size_t size)
{
	if (!write_field_name(jfile, fname))
		return 0;
	if (!write_string(jfile, buf))
		return 0;

	return 1;
}

int json_read_time(struct jfile *jfile, const char *fname, time_t *t)
{
	struct tm tm;

	if (!json_read_tm(jfile, fname, &tm))
		return 0;

	*t = mktime(&tm);
	return 1;
}

int json_write_time(struct jfile *jfile, const char *fname, time_t t)
{
	return json_write_tm(jfile, fname, *localtime(&t));
}

int json_read_tm(struct jfile *jfile, const char *fname, struct tm *tm)
{
	/* RFC-3339 */
	char buf[] = "yyyy-mm-ddThh-mm-ssZ";

	if (!read_field_name(jfile, fname))
		return 0;

	if (skip_space(jfile) != '\"')
		return error(jfile, "Expected '\"' to start time string");

	if (fread(buf, sizeof(buf) - 1, 1, jfile->file) != 1) {
		if (ferror(jfile->file))
			return error(jfile, "%s", strerror(errno));
		else
			return error(jfile, "Cannot match RFC-3339 time string");
	}

	if (fgetc(jfile->file) != '\"')
		return error(jfile, "Expected '\"' to end time string");

	if (strptime(buf, "%Y-%m-%dT%H:%M:%SZ", tm) == NULL)
		return error(jfile, "Cannot parse RFC-3339 time string");

	return 1;
}

int json_write_tm(struct jfile *jfile, const char *fname, struct tm tm)
{
	/* RFC-3339 */
	char buf[] = "yyyy-mm-ddThh-mm-ssZ";
	size_t ret;

	if (!write_field_name(jfile, fname))
		return 0;

	ret = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
	if (ret != sizeof(buf) - 1)
		return error(jfile, "Cannot convert to RFC-3339 time string");

	if (!write_string(jfile, buf))
		return 0;

	return 1;
}
