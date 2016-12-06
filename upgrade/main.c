#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

/* prefix-header is buggy and didn't prefixed this constant */
#define PACKET_HEADER_SIZE 6

#include "config.h"
#include "player.h"
#include "server.h"

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

	const char query[] =
		"INSERT INTO player_historic(name, timestamp, elo, rank)"
		" VALUES (?, ?, ?, ?)";

	for (r = hist->first; r; r = r->next) {
		struct teerank6_player_record *data = teerank6_record_data(hist, r);

		if (!exec(query, "stiu", pname, r->time, data->elo, data->rank))
			return 0;
	}

	return 1;
}

static unsigned char hextodec(char c)
{
	switch (c) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A':
	case 'a': return 10;
	case 'B':
	case 'b': return 11;
	case 'C':
	case 'c': return 12;
	case 'D':
	case 'd': return 13;
	case 'E':
	case 'e': return 14;
	case 'F':
	case 'f': return 15;
	default: return 0;
	}
}

static void hexname_to_name(const char *hex, char *name)
{
	size_t size;

	for (size = 0; hex[0] && hex[1] && size < NAME_LENGTH; hex += 2, name++, size++)
		*name = hextodec(hex[0]) * 16 + hextodec(hex[1]);

	*(name - 1) = '\0';
}

static int upgrade_player(struct teerank6_player *old, struct player *new)
{
	hexname_to_name(old->name, new->name);
	hexname_to_name(old->clan, new->clan);

	new->elo = old->elo;
	new->rank = old->rank;

	strcpy(new->server_ip, old->server_ip);
	strcpy(new->server_port, old->server_port);

	new->lastseen = mktime(&old->lastseen);

	return upgrade_historic(new->name, &old->hist);
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
		hexname_to_name(old->clients[i].name, new->clients[i].name);
		hexname_to_name(old->clients[i].clan, new->clients[i].clan);
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

		if (old.ip && old.port) {
			upgrade_server(&old, &new);
			write_server(&new);
			write_server_clients(&new);
		}
	}

	closedir(dir);
}

int main(int argc, char *argv[])
{
	/* Teerank 3.x use $TEERANK_ROOT to locate database */
	setenv("TEERANK_ROOT", ".teerank", 0);

	load_config(0);
	teerank6_load_config(1);

	/* Non-stable version may not be re-upgradable */
	if (!STABLE_VERSION) {
		char buf[] = "continue";

		fprintf(stderr, "/!\\ WARNING /!\\\n\n");
		fprintf(stderr, "Data are being upgraded to an unstable database format.\n");
		fprintf(stderr, "This format may not be portable to newer stable release.\n\n");
		fprintf(stderr, "Type \"%s\" to proceed: ", buf);
		scanf("%8s", buf);

		if (strcmp(buf, "continue") != 0) {
			fprintf(stderr, "Upgrade canceled\n");
			return EXIT_FAILURE;
		}
	}

	printf("Upgrading from %u to %u\n", DATABASE_VERSION - 1, DATABASE_VERSION);

	exec("BEGIN");

	upgrade_players();
	upgrade_servers();

	exec("COMMIT");

	printf("Success\n");

	return EXIT_SUCCESS;
}
