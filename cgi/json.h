#ifndef JSON_H
#define JSON_H

#include "database.h"
#include "io.h"

struct json_list_column {
	char *title;
	char type[3];
};

void json_list(
	struct list *list, struct json_list_column *cols, char *lenname);
void json_array_list(
	struct list *list, struct json_list_column *cols, char *lenname);
void json_value_list(
	struct list *list, struct json_list_column *cols, char *lenname);

#endif /* JSON_H */
