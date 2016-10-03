#ifndef JSON_H
#define JSON_H

/*
 * 		Overview
 *
 * The main reason to implement our own specialized JSON "parser" is
 * because we need soem features that are not provided by any others.
 * And also because json format is actually quite simple and flexible.
 *
 * First, we need a fast parsing process.  The actual trend is to split
 * the parsing process in two steps: parsing the whole file at first,
 * and then providing an in memory view of the file.
 *
 * 	json_parse("<path_to_file>", &ret);
 *
 * 	if (strcmp(ret.fields[0].name, "name") == 0)
 * 		strcpy(name, ret.fields[0].data.string);
 * 	if (strcmp(ret.fields[1].name, "age") == 0)
 * 		age = ret.fields[1].data.integer;
 *
 * This approach is generic but is not really wanted in our case because
 * we know what field we expect.  It means we can stop the parsing
 * process as soon as we detect a mismatch between the expected field
 * and the currently parsed field.
 *
 * What our JSON parser does it to directly encode file structure at
 * parsing time, like so:
 *
 *	json_init(&jfile, file, path);
 * 	json_read_string(&jfile, "name", name, sizeof(name));
 * 	json_read_uint(&jfile, "age", &age);
 *
 *
 *		Memory allocations
 *
 * Last but not least, we want to avoid memory allocations.  Every
 * buffers of unknown size should be provided by the user, and the
 * parser never allocate memory at any point.
 *
 *
 *		Error handling
 *
 * Our json parser allow batching calls without any error checking, and
 * check for errors at the end.  It makes the code easier to read and
 * reduce the number of (probably untested) possible error path.
 *
 * 	json_read_string(&jfile, "name", name, sizeof(name));
 * 	json_read_uint(&jfile, "age", &age);
 *
 * 	if (json_have_error(&jfile))
 * 		return 0;
 */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_SCOPE_LEVEL 8

/*
 * Keep track of field count so that we can automatically place coma for
 * every object with more than one field.
 *
 * Store a position in the file at wich the object started, so
 * substracting it at the end of the object give the object size.
 *
 * Last but not least, a scope also contain the size of each individual
 * field in this very scope.  This is useful to make field directly
 * accessible by seeking the file.  We call such scope "indexable".
 */
struct json_scope {
	unsigned field_count;
	off_t offset;
};

struct jfile {
	const char *path;
	FILE *file;
	int have_error;

	unsigned scope_level;
	struct json_scope scopes[MAX_SCOPE_LEVEL];
	struct json_scope *scope; /* j->scope[j->scope_level] */
};

/*
 * Return the size of the decimal representation of the given number,
 * including terminating nul character.
 */
#define _strsize(n) (size_t)sizeof(#n)

/* Initialize the json parser with an already opened file */
void json_init(struct jfile *jfile, FILE *file, const char *path);

/*
 * Print the given error message prefixed with filename and actual file
 * position.  Also set error flag.
 */
int json_print_error(struct jfile *jfile, const char *fmt, ...);

/*
 * Return if wether or not an error occured in any previous calls to
 * json_{read,write}_*().
 *
 * The idea is to batch calls to json functions and then check if an
 * error occured, instead of checking each time for errors.
 */
int json_have_error(struct jfile *jfile);

/*
 * Objects
 */

int json_read_object_start(struct jfile *jfile, const char *fname);
size_t json_read_object_end(struct jfile *jfile);

int json_write_object_start(struct jfile *jfile, const char *fname);
size_t json_write_object_end(struct jfile *jfile);

/*
 * Arrays
 */

int json_read_array_start(struct jfile *jfile, const char *fname);
size_t json_read_array_end(struct jfile *jfile);
size_t json_try_read_array_end(struct jfile *jfile);

int json_write_array_start(struct jfile *jfile, const char *fname);
size_t json_write_array_end(struct jfile *jfile);

/*
 * (Unsigned) integer
 */

int json_read_int(struct jfile *jfile, const char *fname, int *buf);
int json_write_int(struct jfile *jfile, const char *fname, int buf);

int json_read_unsigned(struct jfile *jfile, const char *fname, unsigned *buf);
int json_write_unsigned(struct jfile *jfile, const char *fname, unsigned buf);

/*
 * Bool
 */

int json_read_bool(struct jfile *jfile, const char *fname, int *buf);
int json_write_bool(struct jfile *jfile, const char *fname, int buf);

/*
 * Strings
 */

int json_read_string(struct jfile *jfile, const char *fname, char *buf, size_t size);
int json_write_string(struct jfile *jfile, const char *fname, const char *buf, size_t size);

/*
 * Time
 */

int json_read_time(struct jfile *jfile, const char *fname, time_t *t);
int json_write_time(struct jfile *jfile, const char *fname, time_t t);

int json_read_tm(struct jfile *jfile, const char *fname, struct tm *t);
int json_write_tm(struct jfile *jfile, const char *fname, struct tm t);

#endif /* JSON_H */
