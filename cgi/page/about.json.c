#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "teerank.h"
#include "json.h"
#include "master.h"
#include "player.h"
#include "clan.h"
#include "server.h"

static void json_master(struct master *master)
{
	putchar('{');
	printf("\"node\":\"%s\",", json_escape(master->node));
	printf("\"service\":\"%s\",", json_escape(master->node));
	printf("\"last_seen\":\"%s\",", json_date(master->lastseen));
	printf("\"nservers\":%u", master->nservers);
	putchar('}');
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

	putchar('{');

	printf("\"nplayers\":%u,", count_ranked_players("", ""));
	printf("\"nclans\":%u,",   count_clans());
	printf("\"nservers\":%u,", count_vanilla_servers());
	printf("\"last_update\":\"%s\",", json_date(last_database_update()));

	printf("\"masters\":[");

	foreach_extended_master(query, &master) {
		if (nrow)
			putchar(',');
		json_master(&master);
	}

	printf("],\"nmasters\":%u", nrow);
	putchar('}');

	return res ? SUCCESS : FAILURE;
}
