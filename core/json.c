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
		c = getc_unlocked(jfile->file);
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
		c = getc_unlocked(jfile->file);
	}

	if (*fname != '\0')
		return error(jfile, "Expected '\"' to end field name");
	if (getc_unlocked(jfile->file) != ':')
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

	if (jfile->scope->offset == -1 && errno != ESPIPE)
		return error(jfile, "%s", strerror(errno));

	return 1;
}

static size_t scope_size(struct jfile *jfile)
{
	off_t offset;
	size_t size;

	offset = ftello(jfile->file);
	if (offset == -1 && errno != ESPIPE)
		return error(jfile, "%s", strerror(errno));

	size = offset - jfile->scope->offset;

	return size ? size : 1;
}

/*
 * Set scope pointer to the previous scope, compute and return object
 * size.
 */
static size_t decrease_scope_level(struct jfile *jfile)
{
	size_t size;

	assert(jfile->scope_level > 0);

	if ((size = scope_size(jfile)) == 0)
		return 0;

	jfile->scope_level--;
	jfile->scope = &jfile->scopes[jfile->scope_level];

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

	return decrease_scope_level(jfile);
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

	/* The first field does not need to be preceeded by a coma */
	if (jfile->scope->field_count > 0) {
		if (fputc(',', jfile->file) == EOF)
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
	if (fputc('}', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return decrease_scope_level(jfile);
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

	return decrease_scope_level(jfile);
}

size_t json_try_read_array_end(struct jfile *jfile)
{
	char c = skip_space(jfile);

	if (c != ']') {
		ungetc(c, jfile->file);
		return 0;
	}

	return decrease_scope_level(jfile);
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
	if (fputc(']', jfile->file) == EOF)
		return error(jfile, "%s", strerror(errno));

	return decrease_scope_level(jfile);
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

int json_read_bool(struct jfile *jfile, const char *fname, int *buf)
{
	int c;
	const char *toscan;

	if (!read_field_name(jfile, fname))
		return 0;

	c = getc_unlocked(jfile->file);

	if (c == 't') {
		toscan = "rue";
		*buf = 1;
	} else if (c == 'f') {
		toscan = "alse";
		*buf = 0;
	} else {
		return error(jfile, "Expected boolean (true or false)");
	}

	while (*toscan && getc_unlocked(jfile->file) == *toscan)
		toscan++;

	if (*toscan)
		return error(jfile, "Ill-formed boolean value\n");

	return 1;
}

int json_write_bool(struct jfile *jfile, const char *fname, int buf)
{
	int ret;

	if (!write_field_name(jfile, fname))
		return 0;

	if (buf)
		ret = fputs("true", jfile->file);
	else
		ret = fputs("false", jfile->file);

	if (ret == EOF)
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
		c = getc_unlocked(jfile->file);
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
	struct tm tm = { 0 };

	if (!json_read_tm(jfile, fname, &tm))
		return 0;

	*t = mktime(&tm);
	return 1;
}

int json_write_time(struct jfile *jfile, const char *fname, time_t t)
{
	return json_write_tm(jfile, fname, *gmtime(&t));
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

	if (getc_unlocked(jfile->file) != '\"')
		return error(jfile, "Expected '\"' to end time string");

	if (strptime(buf, "%Y-%m-%dT%H:%M:%SZ", tm) == NULL)
		return error(jfile, "Cannot parse RFC-3339 time string");

	return 1;
}

int json_write_tm(struct jfile *jfile, const char *fname, struct tm tm)
{
	/* RFC-3339 */
	char buf[] = "yyyy-mm-ddThh:mm:ssZ";
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
