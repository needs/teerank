#ifndef IO_H
#define IO_H

#include <stdio.h>

/*
 * Teeworlds names cannot be bigger than 16, including terminating nul byte
 * When converting names to hexadecimal, each letter now take two bytes
 * hence the maximum name length is 16 * 2 + 1.
 */
#define MAX_NAME_LENGTH 33

struct tab {
	char *name, *href;
};
extern const struct tab CTF_TAB, ABOUT_TAB;
extern struct tab CUSTOM_TAB;

void print_header(const struct tab *active, char *title, char *search);
void print_footer(void);

void hex_to_string(const char *hex, char *str);
void string_to_hex(const char *str, char *hex);

void html(char *str);

#endif /* IO_H */
