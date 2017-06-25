#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "json.h"

void generate_json_clan(struct url *url)
{
	char *cname = NULL;
	unsigned i;

	const char *query =
		"SELECT players.name"
		" FROM players"
		" WHERE clan IS NOT NULL AND clan IS ?";

	struct json_list_column cols[] = {
		{ "name", "%s" },
		{ NULL }
	};

	for (i = 0; i < url->nargs; i++) {
		if (strcmp(url->args[i].name, "name") == 0)
			cname = url->args[i].val;
	}

	json("{%s:", "members");
	json_value_list(foreach_init(query, "s", cname), cols, "nmembers");
	json("}");
}
