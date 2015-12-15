#ifndef IO_H
#define IO_H

#include <stdio.h>

/*
 * Teeworlds names cannot be bigger than 16, including terminating nul byte
 * When converting names to hexadecimal, each letter now take two bytes
 * hence the maximum name length is 16 * 2 + 1.
 */
#define MAX_NAME_LENGTH 33

int read_file(const char *path, const char *format, ...);
int write_file(const char *path, const char *format, ...);

void print_file(const char *path);

void hex_to_string(const char *hex, char *str);
void string_to_hex(const char *str, char *hex);

void html(char *str);

#endif /* IO_H */
