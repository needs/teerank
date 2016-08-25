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
}

#define error(jfile, fmt...) json_error(jfile, fmt)

int json_error(struct jfile *jfile, const char *fmt, ...)
{
	va_list ap;

	/* Suppose file is a oneliner */
	fprintf(stderr, "%s:1:%ld: ", jfile->path, ftell(jfile->file));

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
	return 0;
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
	const char *str = fname;
	int c;

	if (jfile->field_count[jfile->scope_level] > 0 && skip_space(jfile) != ',')
		return error(jfile, "Expected ',' between two fields");

	jfile->field_count[jfile->scope_level]++;

	if (!fname)
		return 1;

	if (skip_space(jfile) != '\"')
		return error(jfile, "Expected '\"' to start field name");

	goto start;
	while (*str) {
		if (c == '\"')
			break;
		if (c != *str)
			return error(jfile, "Expected field name \"%s\"", fname);
		str++;
	start:
		c = fgetc(jfile->file);
	}

	if (*str)
		return error(jfile, "Expected '\"' to end field name");
	if (fgetc(jfile->file) != ':')
		return error(jfile, "Expected ':' after field name");

	return 1;
}

static void increase_scope_level(struct jfile *jfile)
{
	jfile->scope_level++;
	jfile->field_count[jfile->scope_level] = 0;
}

static void decrease_scope_level(struct jfile *jfile)
{
	assert(jfile->scope_level > 0);
	jfile->scope_level--;
}

int json_read_object_start(struct jfile *jfile, const char *fname)
{
	if (!read_field_name(jfile, fname))
		return 0;
	if (skip_space(jfile) != '{')
		return error(jfile, "Expected '{' to start object");

	increase_scope_level(jfile);

	return 1;
}

int json_read_object_end(struct jfile *jfile)
{
	if (skip_space(jfile) != '}')
		return error(jfile, "Expected '}' to end object");

	decrease_scope_level(jfile);

	return 1;
}

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
	if (jfile->field_count[jfile->scope_level] > 0) {
		if (fputc(',', jfile->file) == EOF)
			return error(jfile, "%s", strerror(errno));
	}

	jfile->field_count[jfile->scope_level]++;

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
	if (fputc('{', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	increase_scope_level(jfile);

	return 1;
}

int json_write_object_end(struct jfile *jfile)
{
	assert(jfile != NULL);
	assert(jfile->scope_level > 0);

	if (fputc('}', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	decrease_scope_level(jfile);

	return 1;
}

int json_read_array_start(struct jfile *jfile, const char *fname)
{
	if (!read_field_name(jfile, fname))
		return 0;
	if (skip_space(jfile) != '[')
		return error(jfile, "Expected '[' to start array");

	increase_scope_level(jfile);

	return 1;
}

int json_read_array_end(struct jfile *jfile)
{
	if (skip_space(jfile) != ']')
		return error(jfile, "Expected ']' to end array");

	decrease_scope_level(jfile);

	return 1;
}

int json_write_array_start(struct jfile *jfile, const char *fname)
{
	if (!write_field_name(jfile, fname))
		return 0;
	if (fputc('[', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	increase_scope_level(jfile);

	return 1;
}

int json_write_array_end(struct jfile *jfile)
{
	assert(jfile != NULL);
	assert(jfile->scope_level > 0);

	if (fputc(']', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	decrease_scope_level(jfile);

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
