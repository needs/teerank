#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "json.h"

void generate_json_about(struct url *url)
{
	struct list list;
	unsigned nrow;

	struct json_list_column cols[] = {
		{ "node", "%s" },
		{ "service", "%s" },
		{ "last_seen", "%d" },
		{ "nservers", "%u" },
		{ NULL }
	};

	const char *query =
		"SELECT node, service, lastseen,"
		" (SELECT COUNT(1)"
		"  FROM server_masters"
		"  WHERE master_node = node"
		"  AND master_service = service)"
		" FROM masters"
		" ORDER BY node";

	json("{");

	nrow = count_rows("SELECT COUNT(1) FROM players");
	json("%s:%u,", "nplayers", nrow);

	nrow = count_rows("SELECT COUNT(1) FROM players GROUP BY clan");
	json("%s:%u,", "nclans", nrow);

	nrow = count_rows("SELECT COUNT(1) FROM servers");
	json("%s:%u,", "nservers", nrow);

	json("%s:%d,", "last_update", last_database_update());

	json("%s:", "masters");
	list = init_simple_list(query, NULL);
	json_list(&list, cols, "nmasters");

	json("}");
}
