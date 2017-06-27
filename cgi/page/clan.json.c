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
	char *cname;

	const char *query =
		"SELECT name"
		" FROM players"
		" WHERE clan IS NOT NULL AND clan = ?";

	struct json_list_column cols[] = {
		{ "name", "%s" },
		{ NULL }
	};

	if (!(cname = URL_EXTRACT(url, PARAM_NAME(0))))
		error(400, "Clan name required");

	json("{%s:", "members");
	json_value_list(foreach_init(query, "s", cname), cols, "nmembers");
	json("}");
}
