#ifndef JSON_H
#define JSON_H

#include <time.h>

/* Maximum level of objects nesting supported */
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
	size_t field_size;
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
#define _strsize(n) (int)sizeof(#n)

/* Add one because we need to count coma between fields */
#define _JSON_FIELD_SIZE(size) ((size) + 1)

/*
 * The worst case is every characters of the string are escaped, thus
 * doubling string size.  We need to add an extra 2 byte for the opening
 * and closing double-quotes.
 */
#define JSON_STRING_SIZE(size) _JSON_FIELD_SIZE((size) * 2 + 2)

/* Variant without any escaped characters */
#define JSON_RAW_STRING_SIZE(size) _JSON_FIELD_SIZE((size) + 2)

/* Add one because negative intergers must be prefixed with '-' */
#define JSON_INT_SIZE _JSON_FIELD_SIZE(_strsize(INTMAX) + 1)

/* Add one because negative intergers must be prefixed with '-' */
#define JSON_UINT_SIZE _JSON_FIELD_SIZE(_strsize(UINTMAX))

/* Size of RFC-3339 date format */
#define JSON_DATE_SIZE JSON_RAW_STRING_SIZE(sizeof("yyyy-mm-ddThh:mm:ssZ"))

/* Two brackets */
#define JSON_ARRAY_SIZE 2

/**
 * Initialize a jfile with an already opened file.
 */
void json_init(struct jfile *jfile, FILE *file, const char *path);

/**
 * Print an error with JSON parsing context
 */
int json_print_error(struct jfile *jfile, const char *fmt, ...);

/**
 * Return if wether or not an error occured in any previous calls to
 * json_{read,write}_*().
 *
 * The idea is to batch calls to json functions and then check if an
 * error occured, instead of checking each time for errors.
 */
int json_have_error(struct jfile *jfile);

/**
 * Read headers of a JSON object
 */
int json_read_object_start(struct jfile *jfile, const char *fname);

/**
 * Read footer of a previsouly started JSON object
 */
size_t json_read_object_end(struct jfile *jfile);

/**
 * Write headers of a JSON object
 */
int json_write_object_start(struct jfile *jfile, const char *fname);

/**
 * Write footer of a previsouly started JSON object
 */
size_t json_write_object_end(struct jfile *jfile);

/**
 * Read headers of a JSON array
 */
int json_read_array_start(struct jfile *jfile, const char *fname);

/**
 * Read footer of a previously started JSON array
 */
size_t json_read_array_end(struct jfile *jfile);

/**
 * Read headers of an indexable JSON array
 */
int json_read_indexable_array_start(struct jfile *jfile, const char *fname, size_t field_size);

/**
 * Read footer of a previously started indexable JSON array
 */
size_t json_read_indexable_array_end(struct jfile *jfile);

/**
 * Write headers of a JSON array
 */
int json_write_array_start(struct jfile *jfile, const char *fname);

/**
 * Write footer of a previsouly started JSON array
 */
size_t json_write_array_end(struct jfile *jfile);

/**
 * Write headers of an indexable JSON array
 */
int json_write_indexable_array_start(
	struct jfile *jfile, const char *fname, size_t entry_size);

/**
 * Write footer of a previsouly started indexable JSON array
 */
size_t json_write_indexable_array_end(struct jfile *jfile);

/**
 * Go to a specific array cell
 */
int json_seek_indexable_array(struct jfile *jfile, unsigned n);

/**
 * Read a signed integer
 */
int json_read_int(struct jfile *jfile, const char *fname, int *buf);

/**
 * Write a signed integer
 */
int json_write_int(struct jfile *jfile, const char *fname, int buf);

/**
 * Read a unsigned integer
 */
int json_read_unsigned(struct jfile *jfile, const char *fname, unsigned *buf);

/**
 * Write a unsigned integer
 */
int json_write_unsigned(struct jfile *jfile, const char *fname, unsigned buf);

/**
 * Read a string of maximum size size (including terminating nul-char)
 */
int json_read_string(struct jfile *jfile, const char *fname, char *buf, size_t size);

/**
 * Write a string
 */
int json_write_string(struct jfile *jfile, const char *fname, const char *buf, size_t size);

/**
 * Read a RFC-3339 time, time_t
 */
int json_read_time(struct jfile *jfile, const char *fname, time_t *t);

/**
 * Write a RFC-3339 time, time_t
 */
int json_write_time(struct jfile *jfile, const char *fname, time_t t);

/**
 * Read a RFC-3339 time, struct tm
 */
int json_read_tm(struct jfile *jfile, const char *fname, struct tm *t);

/**
 * Write a RFC-3339 time, struct tm
 */
int json_write_tm(struct jfile *jfile, const char *fname, struct tm t);

#endif /* JSON_H */
