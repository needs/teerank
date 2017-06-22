#include <assert.h>
#include <stdbool.h>

#include "json.h"
#include "io.h"

enum list_type {
	OBJECT,
	ARRAY,
	VALUE
};

static void list_item(sqlite3_stmt *res, struct json_list_column *col, enum list_type type, unsigned nrow)
{
	unsigned i;

	if(nrow)
		json(",");

	if (type == OBJECT)
		json("{");
	else if (type == ARRAY)
		json("[");

	for (i = 0; col->title; col++, i++) {
		if (i)
			json(",");

		if (type == OBJECT)
			json("%s:", col->title);

		if (is_column_null(res, i)) {
			json("null");
			continue;
		}

		switch (col->type[1]) {
		case 's':
			json(col->type, column_text(res, i));
			break;
		case 'b':
			json(col->type, column_bool(res, i));
			break;
		case 'i':
			json(col->type, column_int(res, i));
			break;
		case 'u':
			json(col->type, column_unsigned(res, i));
			break;
		case 'd':
			json(col->type, column_time_t(res, i));
			break;
		default:
			assert(!"Unimplemented conversion specifier");
		};
	}

	if (type == OBJECT)
		json("}");
	else if (type == ARRAY)
		json("]");
}

static void list(
	sqlite3_stmt *res, struct json_list_column *cols,
	char *lenname, enum list_type type)
{
	unsigned nrow = 0;

	json("[");
	while ((res = foreach_next(res))) {
		list_item(res, cols, type, nrow);
		nrow++;
	}
	json("]");

	if (lenname)
		json(",%s:%u", lenname, nrow);
}

void json_list(
	sqlite3_stmt *res, struct json_list_column *cols, char *lenname)
{
	list(res, cols, lenname, OBJECT);
}
void json_array_list(
	sqlite3_stmt *res, struct json_list_column *cols, char *lenname)
{
	list(res, cols, lenname, ARRAY);
}
void json_value_list(
	sqlite3_stmt *res, struct json_list_column *cols, char *lenname)
{
	list(res, cols, lenname, VALUE);
}
