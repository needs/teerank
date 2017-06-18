#include <assert.h>
#include <stdbool.h>

#include "json.h"
#include "io.h"

enum list_type {
	OBJECT,
	ARRAY,
	VALUE
};

struct args {
	struct json_list_column *cols;
	unsigned nrow;
	enum list_type type;
};

static void list_item(sqlite3_stmt *res, void *args_)
{
	struct args *args = args_;
	struct json_list_column *col = args->cols;
	unsigned i;

	if(args->nrow)
		json(",");

	if (args->type == OBJECT)
		json("{");
	else if (args->type == ARRAY)
		json("[");

	for (i = 0; col->title; col++, i++) {
		if (i)
			json(",");

		if (args->type == OBJECT)
			json("%s:", col->title);

		if (sqlite3_column_type(res, i) == SQLITE_NULL) {
			json("null");
			continue;
		}

		switch (col->type[1]) {
		case 's':
			json(col->type, sqlite3_column_text(res, i));
			break;
		case 'b':
		case 'i':
			json(col->type, sqlite3_column_int(res, i));
			break;
		case 'u':
			json(col->type, (unsigned)sqlite3_column_int64(res, i));
			break;
		case 'd':
			json(col->type, (time_t)sqlite3_column_int64(res, i));
			break;
		default:
			assert(!"Unimplemented conversion specifier");
		};
	}

	if (args->type == OBJECT)
		json("}");
	else if (args->type == ARRAY)
		json("]");
}

static void list(
	sqlite3_stmt *res, struct json_list_column *cols,
	char *lenname, enum list_type type)
{
	struct args args = {
		cols, 0, type
	};

	json("[");
	while (foreach_next(&res, &args, list_item))
		args.nrow++;
	json("]");

	if (lenname)
		json(",%s:%u", lenname, args.nrow);
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
