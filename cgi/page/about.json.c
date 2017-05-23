#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "html.h"
#include "master.h"
#include "player.h"
#include "clan.h"
#include "server.h"

static void json_master(struct master *master)
{
	json("{");
	json("%s:%s,", "node", master->node);
	json("%s:%s,", "service", master->node);
	json("%s:%d,", "last_seen", master->lastseen);
	json("%s:%u", "nservers", master->nservers);
	json("}");
}

int main_json_about(struct url *url)
{
	unsigned nrow = 0;
	sqlite3_stmt *res;
	struct master master;

	const char *query =
		"SELECT" ALL_EXTENDED_MASTER_COLUMNS
		" FROM masters"
		" ORDER BY node";

	json("{");

	json("%s:%u,", "nplayers", count_ranked_players("", ""));
	json("%s:%u,", "nclans", count_clans());
	json("%s:%u,", "nservers", count_vanilla_servers());
	json("%s:%d,", "last_update", last_database_update());

	json("%s:[", "masters");

	foreach_extended_master(query, &master) {
		if (nrow)
			json(",");
		json_master(&master);
	}

	json("],%s:%u", "nmasters", nrow);
	json("}");

	return res ? SUCCESS : FAILURE;
}
