#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>

#include <netinet/in.h>

#include "network.h"
#include "pool.h"
#include "delta.h"
#include "io.h"
#include "config.h"
#include "server.h"

static const uint8_t MSG_GETINFO[] = {
	255, 255, 255, 255, 'g', 'i', 'e', '3'
};
static const uint8_t MSG_INFO[] = {
	255, 255, 255, 255, 'i', 'n', 'f', '3'
};

struct unpacker {
	struct data *data;
	size_t offset;
};

static void init_unpacker(struct unpacker *up, struct data *data)
{
	assert(up != NULL);
	assert(data != NULL);

	up->data = data;
	up->offset = 0;
}

static int can_unpack(struct unpacker *up, unsigned length)
{
	unsigned offset;

	assert(up != NULL);
	assert(length > 0);

	for (offset = up->offset; offset < up->data->size; offset++)
		if (up->data->buffer[offset] == 0)
			if (--length == 0)
				return 1;

	fprintf(stderr, "can_unpack(): %u field(s) missing\n", length);
	return 0;
}

static char *unpack_string(struct unpacker *up)
{
	size_t old_offset;

	assert(up != NULL);

	old_offset = up->offset;
	while (up->offset < up->data->size
	       && up->data->buffer[up->offset] != 0)
		up->offset++;

	/* Skip the remaining 0 */
	up->offset++;

	/* can_unpack() should have been used */
	assert(up->offset <= up->data->size);

	return (char*)&up->data->buffer[old_offset];
}

static long int unpack_int(struct unpacker *up)
{
	long ret;
	char *str, *endptr;

	assert(up != NULL);

	str = unpack_string(up);
	errno = 0;
	ret = strtol(str, &endptr, 10);

	if (errno == ERANGE && ret == LONG_MIN)
		fprintf(stderr, "unpack_int(%s): Underflow, value truncated\n", str);
	else if (errno == ERANGE && ret == LONG_MAX)
		fprintf(stderr, "unpack_int(%s): Overflow, value truncated\n", str);
	else if (endptr == str)
		fprintf(stderr, "unpack_int(%s): Cannot convert string\n", str);
	return ret;
}

static int validate_client_info(struct client *client)
{
	char name[MAX_NAME_HEX_LENGTH];
	char clan[MAX_CLAN_HEX_LENGTH];

	assert(client != NULL);
	assert(client->name != NULL);
	assert(client->clan != NULL);

	if (strlen(client->name) >= MAX_NAME_STR_LENGTH)
		return 0;
	if (strlen(client->clan) >= MAX_CLAN_STR_LENGTH)
		return 0;

	string_to_hex(client->name, name);
	strcpy(client->name, name);
	string_to_hex(client->clan, clan);
	strcpy(client->clan, clan);

	return 1;
}

static int validate_clients_info(struct server_state *state)
{
	unsigned i;

	assert(state != NULL);

	for (i = 0; i < state->num_clients; i++)
		if (!validate_client_info(&state->clients[i]))
			return 0;
	return 1;
}

static int unpack_server_state(struct data *data, struct server_state *state)
{
	struct unpacker up;
	unsigned i;

	assert(data != NULL);
	assert(state != NULL);

	/* Unpack server state (ignore useless infos) */
	init_unpacker(&up, data);
	if (!can_unpack(&up, 10))
		return 0;

	unpack_string(&up);     /* Token */
	unpack_string(&up);     /* Version */
	unpack_string(&up);     /* Name */
	unpack_string(&up);     /* Map */
	state->gametype = unpack_string(&up);     /* Gametype */

	unpack_string(&up);     /* Flags */
	unpack_string(&up);     /* Player number */
	unpack_string(&up);     /* Player max number */
	state->num_clients = unpack_int(&up);     /* Client number */
	unpack_string(&up);     /* Client max number */

	if (state->num_clients > MAX_CLIENTS) {
		fprintf(stderr, "num_clients shouldn't be higher than %d\n",
		        MAX_CLIENTS);
		return 0;
	}

	/* Players */
	for (i = 0; i < state->num_clients; i++) {
		if (!can_unpack(&up, 5))
			return 0;

		strcpy(state->clients[i].name, unpack_string(&up)); /* Name */
		strcpy(state->clients[i].clan, unpack_string(&up)); /* Clan */
		unpack_string(&up); /* Country */
		state->clients[i].score  = unpack_int(&up); /* Score */
		state->clients[i].ingame = unpack_int(&up); /* Ingame? */
	}

	if (up.offset != up.data->size) {
		fprintf(stderr,
		        "%lu bytes remaining after unpacking server state\n",
		        up.data->size - up.offset);
	}

	if (!validate_clients_info(state))
		return 0;

	return 1;
}

struct server {
	char dirname[PATH_MAX];
	struct sockaddr_storage addr;

	struct server_meta meta;

	struct pool_entry entry;
};

struct server_list {
	unsigned length;
	struct server *servers;
};

static int swap_server_state(
	struct server *server,
	struct server_state *old, struct server_state *new)
{
	assert(server != NULL);
	assert(old != NULL);
	assert(new != NULL);
	assert(!strcmp(new->gametype, "CTF"));

	if (!read_server_state(old, server->dirname))
		return 0;
	if (!write_server_state(new, server->dirname))
		return 0;

	return 1;
}

static struct client *get_player(
	struct server_state *state, struct client *client)
{
	unsigned i;

	assert(state != NULL);
	assert(client != NULL);

	for (i = 0; i < state->num_clients; i++)
		if (!strcmp(state->clients[i].name, client->name))
			return &state->clients[i];

	return NULL;
}

static void print_server_state_delta(
	struct server_state *old, struct server_state *new, int elapsed)
{
	struct delta delta;
	unsigned i;

	assert(old != NULL);
	assert(new != NULL);
	assert(!strcmp(new->gametype, "CTF"));

	/* A NULL gametype means empty old server states */
	if (!old->gametype)
		return;
	assert(!strcmp(old->gametype, "CTF"));

	delta.elapsed = elapsed;
	delta.length = 0;
	for (i = 0; i < new->num_clients; i++) {
		struct client *old_player, *new_player;

		new_player = &new->clients[i];
		old_player = get_player(old, new_player);

		if (old_player) {
			struct player_delta *player;
			player = &delta.players[delta.length];
			strcpy(player->name, new_player->name);
			strcpy(player->clan, new_player->clan);
			player->score = new_player->score;
			player->delta = new_player->score - old_player->score;
			delta.length++;
		}
	}

	print_delta(&delta);
}

static void remove_spectators(struct server_state *state)
{
	unsigned i;

	assert(state != NULL);

	for (i = 0; i < state->num_clients; i++) {
		if (!state->clients[i].ingame) {
			state->clients[i] = state->clients[--state->num_clients];
			i--;
		}
	}
}

static int handle_data(struct data *data, struct server *server)
{
	struct server_state old, new;

	assert(data != NULL);
	assert(server != NULL);

	if (!skip_header(data, MSG_INFO, sizeof(MSG_INFO)))
		return 0;
	if (!unpack_server_state(data, &new))
		return 0;

	if (strcmp(new.gametype, "CTF")) {
		/*
		 * We don't rank this server but we still want to check
		 * it time to time to see if its gametype change.
		 */
		mark_server_online(&server->meta, 0);
		write_server_meta(&server->meta, server->dirname);
	} else {
		int elapsed = time(NULL) - server->meta.last_seen;
		mark_server_online(&server->meta, 1);
		write_server_meta(&server->meta, server->dirname);

		remove_spectators(&new);
		if (!swap_server_state(server, &old, &new))
			return 0;

		print_server_state_delta(&old, &new, elapsed);
	}

	return 1;
}

#define _str(s) #s
#define str(s) _str(s)

static int extract_ip_and_port(char *name, char *ip, char *port)
{
	char c;
	unsigned i, version;
	int ret;

	assert(ip != NULL);
	assert(port != NULL);

	errno = 0;
	ret = sscanf(name, "v%u %" str(IP_LENGTH) "s %" str(PORT_LENGTH) "s",
	             &version, ip, port);
	if (ret == EOF && errno != 0)
		return perror(name), 0;
	else if (ret == EOF && errno == 0)
		return fprintf(stderr, "%s: Cannot find IP version\n", name), 0;
	else if (ret == 1)
		return fprintf(stderr, "%s: Cannot find IP\n", name), 0;
	else if (ret == 2)
		return fprintf(stderr, "%s: Cannot find port\n", name), 0;

	/* Replace '_' by '.' or ':' depending on IP version */
	if (version == 4)
		c = '.';
	else if (version == 6)
		c = ':';
	else {
		fprintf(stderr, "%s: IP version should be either 4 or 6\n", name);
		return 0;
	}

	for (i = 0; i < strlen(ip); i++)
		if (ip[i] == '_')
			ip[i] = c;

	/* IP and port are implicitely checked later in get_sockaddr() */
	return 1;
}

static int add_server(struct server_list *list, struct server *server)
{
	static const unsigned OFFSET = 1024;

	if (list->length % OFFSET == 0) {
		struct server *servers;
		servers = realloc(list->servers, sizeof(*servers) * (list->length + OFFSET));
		if (!servers)
			return 0;
		list->servers = servers;
	}

	list->servers[list->length++] = *server;
	return 1;
}

static int fill_server_list(struct server_list *list)
{
	DIR *dir;
	struct dirent *dp;
	char path[PATH_MAX];
	unsigned count = 0;

	assert(list != NULL);

	sprintf(path, "%s/servers", config.root);
	if (!(dir = opendir(path)))
		return perror(path), 0;

	/* Fill array (ignore server on error) */
	while ((dp = readdir(dir))) {
		char ip[IP_LENGTH + 1], port[PORT_LENGTH + 1];
		struct server server;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		count++;
		/*
		 * By reading server metadata we are able to filter-out
		 * servers that has not the wanted gamemode or servers
		 * that do not need to be refreshed often.
		 */
		read_server_meta(&server.meta, dp->d_name);
		if (!server_need_refresh(&server.meta))
			continue;

		strcpy(server.dirname, dp->d_name);
		if (!extract_ip_and_port(dp->d_name, ip, port))
			continue;
		if (!get_sockaddr(ip, port, &server.addr))
			continue;
		if (!add_server(list, &server))
			continue;
	}

	closedir(dir);

	verbose("%u servers found, %u will be refreshed\n",
	        count, list->length);

	return 1;
}

static struct server *get_server(struct pool_entry *entry)
{
	assert(entry != NULL);
	return (struct server*)((char*)entry - offsetof(struct server, entry));
}

static void poll_servers(struct server_list *list, struct sockets *sockets)
{
	const struct data request = {
		sizeof(MSG_GETINFO) + 1, {
			MSG_GETINFO[0], MSG_GETINFO[1], MSG_GETINFO[2],
			MSG_GETINFO[3], MSG_GETINFO[4], MSG_GETINFO[5],
			MSG_GETINFO[6], MSG_GETINFO[7], 0
		}
	};
	struct pool pool;
	struct pool_entry *entry;
	struct data answer;
	unsigned i, failed_count = 0;

	assert(list != NULL);
	assert(sockets != NULL);

	init_pool(&pool, sockets, &request);
	for (i = 0; i < list->length; i++)
		add_pool_entry(&pool, &list->servers[i].entry,
		               &list->servers[i].addr);

	while ((entry = poll_pool(&pool, &answer)))
		handle_data(&answer, get_server(entry));

	while ((entry = foreach_failed_poll(&pool))) {
		struct server *server = get_server(entry);
		mark_server_offline(&server->meta);
		write_server_meta(&server->meta, server->dirname);
		failed_count++;
	}

	verbose("%u servers has not been successfully polled\n", failed_count);
}

static const struct server_list SERVER_LIST_ZERO;

int main(int argc, char **argv)
{
	struct sockets sockets;
	struct server_list list = SERVER_LIST_ZERO;

	load_config();
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!fill_server_list(&list))
		return EXIT_FAILURE;

	if (!init_sockets(&sockets))
		return EXIT_FAILURE;
	poll_servers(&list, &sockets);
	close_sockets(&sockets);

	return EXIT_SUCCESS;
}
