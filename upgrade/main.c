#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include "config.h"
#include "index.h"
#include "player.h"
#include "server.h"
#include "clan.h"

#include "teerank5/core/config.h"
#include "teerank5/core/player.h"
#include "teerank5/core/server.h"
#include "teerank5/core/clan.h"

static int sort_by_elo(const void *pa, const void *pb)
{
	const struct indexed_player *a = pa, *b = pb;

	if (a->elo < b->elo)
		return 1;
	else if (a->elo == b->elo)
		return 0;
	else
		return -1;
}

static void create_players_by_rank_file(void)
{
	struct index index;

	if (!create_index(&index, &INDEX_DATA_INFO_PLAYER, NULL))
		exit(EXIT_FAILURE);

	sort_index(&index, sort_by_elo);

	if (!write_index(&index, "players_by_rank")) {
		close_index(&index);
		exit(EXIT_FAILURE);
	}

	close_index(&index);
}

static void remove_ranks_file(void)
{
	char path[PATH_MAX];

	if (!dbpath(path, PATH_MAX, "ranks"))
		exit(EXIT_FAILURE);

	if (unlink(path) == -1 && errno != ENOENT) {
		perror(path);
		exit(EXIT_FAILURE);
	}
}

static void upgrade_historic(struct teerank5_historic *old, struct historic *new)
{
	new->epoch = old->epoch;
	new->nrecords = old->nrecords;
	new->max_records = old->max_records;
	new->length = old->length;
	new->records = (struct record*)old->records;
	new->first = (struct record*)old->first;
	new->last = (struct record*)old->last;
	new->data_size = old->data_size;
	new->data = old->data;
}

static void upgrade_player(struct teerank5_player *old, struct player *new)
{
	memcpy(new->name, old->name, sizeof(new->name));
	memcpy(new->clan, old->clan, sizeof(new->clan));

	new->elo = old->elo;
	new->rank = old->rank;

	/*
	 * Historic structs hasn't changed at all, so it should be safe
	 * to copy them like that.
	 */
	upgrade_historic(&old->hist, &new->hist);

	/* The only change is the added lastseen field */
	new->lastseen = *gmtime(&NEVER_SEEN);
}

static void upgrade_players(void)
{
	struct teerank5_player old;
	struct player new;

	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	if (!dbpath(path, PATH_MAX, "players"))
		exit(EXIT_FAILURE);

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	teerank5_init_player(&old);
	init_player(&new);

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		teerank5_read_player(&old, dp->d_name);
		upgrade_player(&old, &new);
		write_player(&new);
	}

	closedir(dir);
}

static const char *strncpy_until(char *dst, const char *src, size_t maxsize, char sep)
{
	size_t size;

	for (size = 0; *src && *src != sep && size < maxsize; src++, dst++, size++)
		*dst = *src;

	/* Make sure result string is always nul-terminated */
	*dst = '\0';

	if (*src && *src != sep)
		return NULL;

	return *src ? src + 1 : src;
}

static int deconstruct_server_filename(
	const char *fn, char *ip, char *port)
{
	char sep;

	if (fn[0] == 'v' && fn[1] == '4' && fn[2] == ' ') {
		sep = '.';
	} else if (fn[0] == 'v' && fn[1] == '6' && fn[2] == ' ') {
		sep = ':';
	} else {
		fprintf(stderr, "%s: Should start by \"v4 \" or \"v6 \"\n", fn);
		return 0;
	}

	if (!(fn = strncpy_until(ip, fn + 3, IP_STRSIZE, ' '))) {
		fprintf(stderr, "%s: IP too long\n", fn);
		return 0;
	}

	if (!(fn = strncpy_until(port, fn, PORT_STRSIZE, ' '))) {
		fprintf(stderr, "%s: Port too long\n", fn);
		return 0;
	}

	/* Make sure there is no garbage after server port */
	if (*fn) {
		fprintf(stderr, "%s: Extra data after IP and port\n", fn);
		return 0;
	}

	/*
	 * '.' or ':' in IP adress were replaced by '_' because ':' is
	 * forbidden in filename.  Revert it.
	 */
	while ((ip = strchr(ip, '_')))
		*ip = sep;

	return 1;
}

static int upgrade_server(
	const char *filename, struct teerank5_server_state *old, struct server *new)
{
	unsigned i;

	/* Infer IP and port from server name */
	if (!deconstruct_server_filename(filename, new->ip, new->port))
		return 0;

	/* When a field is new, fill it with "???" */
	strcpy(new->name, "???");
	strcpy(new->gametype, "CTF");
	strcpy(new->map, "???");

	new->lastseen = old->last_seen;
	new->expire = old->expire;

	new->num_clients = old->num_clients;
	new->max_clients = MAX_CLIENTS;

	for (i = 0; i < old->num_clients; i++) {
		strcpy(new->clients[i].name, old->clients[i].name);
		strcpy(new->clients[i].clan, old->clients[i].clan);
		new->clients[i].score = old->clients[i].score;
		new->clients[i].ingame = old->clients[i].ingame;
	}

	return 1;
}

static void upgrade_servers(void)
{
	struct teerank5_server_state old;
	struct server new;

	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	if (!dbpath(path, PATH_MAX, "servers"))
		exit(EXIT_FAILURE);

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		teerank5_read_server_state(&old, dp->d_name);

		if (upgrade_server(dp->d_name, &old, &new))
			write_server(&new);
	}

	closedir(dir);
}

static void upgrade_clan(struct teerank5_clan *old, struct clan *new)
{
	unsigned i;

	create_clan(new, old->name, old->length);

	for (i = 0; i < old->length; i++)
		add_member(new, old->members[i].name);
}

static void upgrade_clans(void)
{
	struct teerank5_clan old;
	struct clan new;

	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir;

	if (!dbpath(path, PATH_MAX, "clans"))
		exit(EXIT_FAILURE);

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		teerank5_read_clan(&old, dp->d_name);
		upgrade_clan(&old, &new);
		write_clan(&new);
	}

	closedir(dir);
}

int main(int argc, char *argv[])
{
	load_config(0);
	teerank5_load_config(1);

	remove_ranks_file();
	upgrade_players();
	upgrade_servers();
	upgrade_clans();
	create_players_by_rank_file();

	return EXIT_SUCCESS;
}
