#ifndef JSON_H
#define JSON_H

#include "database.h"

struct json_list_column {
	char *title;
	char type[3];
};

void json_list(
	sqlite3_stmt *res, struct json_list_column *cols, char *lenname);
void json_array_list(
	sqlite3_stmt *res, struct json_list_column *cols, char *lenname);
void json_value_list(
	sqlite3_stmt *res, struct json_list_column *cols, char *lenname);

#endif /* JSON_H */
