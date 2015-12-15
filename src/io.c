#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "io.h"

int read_file(const char *path, const char *format, ...)
{
	FILE *file;
	va_list ap;
	int ret;

	if (!(file = fopen(path, "r")))
		return -1;

	va_start(ap, format);
	ret = vfscanf(file, format, ap);
	va_end(ap);

	fclose(file);
	return ret;
}

int write_file(const char *path, const char *format, ...)
{
	FILE *file;
	va_list ap;
	int ret;

	if (!(file = fopen(path, "w")))
		return -1;

	va_start(ap, format);
	ret = vfprintf(file, format, ap);
	va_end(ap);

	fclose(file);
	return ret;
}

void print_file(const char *path)
{
	FILE *file;
	int c;

	if (!(file = fopen(path, "r")))
		exit(EXIT_FAILURE);

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
}

void fprint_file(FILE *dest, const char *path)
{
	FILE *file;
	int c;

	if (!(file = fopen(path, "r")))
		exit(EXIT_FAILURE);

	while ((c = fgetc(file)) != EOF)
		fputc(c, dest);

	fclose(file);
}

void hex_to_string(const char *hex, char *str)
{
	assert(hex != NULL);
	assert(str != NULL);
	assert(hex != str);

	for (; hex[0] != '0' || hex[1] != '0'; hex += 2, str++) {
		char tmp[3] = { hex[0], hex[1], '\0' };
		*str = strtol(tmp, NULL, 16);
	}

	*str = '\0';
}

void string_to_hex(const char *str, char *hex)
{
	assert(str != NULL);
	assert(hex != NULL);
	assert(str != hex);

	for (; *str; str++, hex += 2)
		sprintf(hex, "%2x", *str);
	strcpy(hex, "00");
}

void html(char *str)
{
	assert(str != NULL);

	do {
		switch (*str) {
		case '<':
			fputs("&lt;", stdout); break;
		case '>':
			fputs("&gt;", stdout); break;
		case '&':
			fputs("&amp;", stdout); break;
		case '"':
			fputs("&quot;", stdout); break;
		default:
			putchar(*str);
		}
	} while (*str++);
}

void fhtml(FILE *file, char *str)
{
	assert(str != NULL);
	assert(file != NULL);

	do {
		switch (*str) {
		case '<':
			fputs("&lt;", file); break;
		case '>':
			fputs("&gt;", file); break;
		case '&':
			fputs("&amp;", file); break;
		case '"':
			fputs("&quot;", file); break;
		default:
			fputc(*str, file);
		}
	} while (*str++);
}
