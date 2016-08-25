#ifndef JSON_H
#define JSON_H

#include <time.h>

/* Maximum level of objects nesting supported */
#define MAX_SCOPE_LEVEL 8

struct jfile {
	const char *path;
	FILE *file;

	/*
	 * We keep track of field count on each scope level because the
	 * first field does not need to be preceded by a comma.
	 */
	unsigned scope_level;
	unsigned field_count[MAX_SCOPE_LEVEL];
};

/**
 * Initialize a jfile with an already opened file.
 */
void json_init(struct jfile *jfile, FILE *file, const char *path);

/**
 * Print an error with JSON parsing context
 */
int json_error(struct jfile *jfile, const char *fmt, ...);

/**
 * Read headers of a JSON object
 */
int json_read_object_start(struct jfile *jfile, const char *fname);

/**
 * Read footer of a previsouly started JSON object
 */
int json_read_object_end(struct jfile *jfile);

/**
 * Write headers of a JSON object
 */
int json_write_object_start(struct jfile *jfile, const char *fname);

/**
 * Write footer of a previsouly started JSON object
 */
int json_write_object_end(struct jfile *jfile);

/**
 * Read headers of a JSON array
 */
int json_read_array_start(struct jfile *jfile, const char *fname);

/**
 * Read footer of a previsouly started JSON array
 */
int json_read_array_end(struct jfile *jfile);

/**
 * Write headers of a JSON array
 */
int json_write_array_start(struct jfile *jfile, const char *fname);

/**
 * Write footer of a previsouly started JSON array
 */
int json_write_array_end(struct jfile *jfile);

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
