#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include "config.h"
#include "player.h"
#include "server.h"
#include "network.h" /* Because prefix-header is buggy */

#include "teerank6/core/config.h"
#include "teerank6/core/player.h"
#include "teerank6/core/server.h"
#include "teerank6/core/clan.h"

/*
 * Copy the whole historic in the table "player_historic".
 */
static int upgrade_historic(const char *pname, struct teerank6_historic *hist)
{
	struct teerank6_record *r;
	sqlite3_stmt *res;
	const char query[] =
		"INSERT INTO player_historic(name, timestamp, elo, rank)"
		" VALUES (?, ?, ?, ?)";

	/*
	 * The query is the same, only values change, so we can reuse
	 * the prepared statement.
	 */
	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	for (r = hist->first; r; r = r->next) {
		struct teerank6_player_record *data = teerank6_record_data(hist, r);

		if (sqlite3_bind_text(res, 1, pname, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_int64(res, 2, r->time) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_int(res, 3, data->elo) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_int64(res, 4, data->rank) != SQLITE_OK)
			goto fail;

		if (sqlite3_step(res) != SQLITE_DONE)
			goto fail;

		if (sqlite3_reset(res) != SQLITE_OK)
			goto fail;
		if (sqlite3_clear_bindings(res) != SQLITE_OK)
			goto fail;
	}

	sqlite3_finalize(res);
	return 1;

fail:
	fprintf(
		stderr, "%s: upgrade_historic(%s): %s\n",
		config.dbpath, pname, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	return 0;
}

static int upgrade_player(struct teerank6_player *old, struct player *new)
{
	memcpy(new->name, old->name, sizeof(new->name));
	memcpy(new->clan, old->clan, sizeof(new->clan));

	new->elo = old->elo;
	new->rank = old->rank;

	strcpy(new->server_ip, old->server_ip);
	strcpy(new->server_port, old->server_port);

	new->lastseen = mktime(&old->lastseen);

	return upgrade_historic(old->name, &old->hist);
}

static void upgrade_players(void)
{
	struct teerank6_player old;
	struct player new;

	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	if (!teerank6_dbpath(path, PATH_MAX, "players"))
		exit(EXIT_FAILURE);

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	teerank6_init_player(&old);

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		teerank6_read_player(&old, dp->d_name);
		if (upgrade_player(&old, &new))
			write_player(&new);
	}

	closedir(dir);
}

static void upgrade_server(
	struct teerank6_server *old, struct server *new)
{
	unsigned i;

	strcpy(new->ip, old->ip);
	strcpy(new->port, old->port);
	strcpy(new->name, old->name);
	strcpy(new->gametype, old->gametype);
	strcpy(new->map, old->map);

	new->lastseen = old->lastseen;
	new->expire = old->expire;

	strcpy(new->master_node, "");
	strcpy(new->master_service, "");

	new->num_clients = old->num_clients;
	new->max_clients = old->max_clients;

	for (i = 0; i < new->num_clients; i++) {
		strcpy(new->clients[i].name, old->clients[i].name);
		strcpy(new->clients[i].clan, old->clients[i].clan);
		new->clients[i].score = old->clients[i].score;
		new->clients[i].ingame = old->clients[i].ingame;
	}
}

static void upgrade_servers(void)
{
	struct teerank6_server old;
	struct server new;

	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	if (!teerank6_dbpath(path, PATH_MAX, "servers"))
		exit(EXIT_FAILURE);

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		teerank6_read_server(&old, dp->d_name);
		upgrade_server(&old, &new);
		write_server(&new);
		write_server_clients(&new);
	}

	closedir(dir);
}

int main(int argc, char *argv[])
{
	load_config(0);
	teerank6_load_config(1);

	sqlite3_exec(db, "BEGIN", 0, 0, 0);

	upgrade_players();
	upgrade_servers();

	sqlite3_exec(db, "COMMIT", 0, 0, 0);

	return EXIT_SUCCESS;
}
