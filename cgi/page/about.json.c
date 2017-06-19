#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "master.h"
#include "clan.h"
#include "server.h"
#include "json.h"

void generate_json_about(struct url *url)
{
	unsigned nrow;
	struct json_list_column cols[] = {
		{ "node", "%s" },
		{ "service", "%s" },
		{ "last_seen", "%d" },
		{ "nservers", "%u" },
		{ NULL }
	};

	const char *query =
		"SELECT node, service, lastseen," NSERVERS_COLUMN
		" FROM masters"
		" ORDER BY node";

	json("{");

	nrow = count_rows("SELECT COUNT(1) FROM players");
	json("%s:%u,", "nplayers", nrow);
	json("%s:%u,", "nclans", count_clans());
	json("%s:%u,", "nservers", count_vanilla_servers());
	json("%s:%d,", "last_update", last_database_update());

	json("%s:", "masters");
	json_list(foreach_init(query, ""), cols, "nmasters");

	json("}");
}
