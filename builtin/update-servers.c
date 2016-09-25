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
#include "config.h"
#include "server.h"
#include "player.h"

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

	return 0;
}

static char *unpack(struct unpacker *up)
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

static void skip_field(struct unpacker *up)
{
	unpack(up);
}

/*
 * It's a shame POSIX do not have a good enough function to copy strings
 * with bound check.  We try to circumvent the lack of such function by
 * using strncat().
 */
static void unpack_string(struct unpacker *up, char *buf, size_t size)
{
	*buf = '\0';
	strncat(buf, unpack(up), size - 1);
}

static long int unpack_int(struct unpacker *up)
{
	long ret;
	char *str, *endptr;

	assert(up != NULL);

	str = unpack(up);
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
	char name[HEXNAME_LENGTH];
	char clan[HEXNAME_LENGTH];

	assert(client != NULL);
	assert(client->name != NULL);
	assert(client->clan != NULL);

	if (strlen(client->name) >= NAME_LENGTH)
		return 0;
	if (strlen(client->clan) >= NAME_LENGTH)
		return 0;

	name_to_hexname(client->name, name);
	strcpy(client->name, name);
	name_to_hexname(client->clan, clan);
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

	skip_field(&up);     /* Token */
	skip_field(&up);     /* Version */
	unpack_string(&up, state->name, sizeof(state->name)); /* Name */
	unpack_string(&up, state->map, sizeof(state->map)); /* Map */
	unpack_string(&up, state->gametype, sizeof(state->gametype)); /* Gametype */

	skip_field(&up);     /* Flags */
	skip_field(&up);     /* Player number */
	skip_field(&up);     /* Player max number */
	state->num_clients = unpack_int(&up); /* Client number */
	state->max_clients = unpack_int(&up); /* Client max number */

	/* Players */
	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		if (!can_unpack(&up, 5))
			return 0;

		/* Name */
		unpack_string(&up, client->name, sizeof(client->name));
		/* Clan */
		unpack_string(&up, client->clan, sizeof(client->clan));

		skip_field(&up); /* Country */
		client->score  = unpack_int(&up); /* Score */
		client->ingame = unpack_int(&up); /* Ingame? */
	}

	if (!validate_clients_info(state))
		return 0;

	return 1;
}

struct netserver {
	char filename[PATH_MAX];
	struct sockaddr_storage addr;

	struct server_state state;

	struct pool_entry entry;
};

struct netserver_list {
	unsigned length;
	struct netserver *netservers;
};

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

static int is_vanilla(struct server_state *state)
{
	if (strcmp(state->gametype, "CTF") != 0
	    && strcmp(state->gametype, "DM") != 0
	    && strcmp(state->gametype, "TDM") != 0)
		return 0;

	if (strcmp(state->map, "ctf1") != 0
	    && strcmp(state->map, "ctf2") != 0
	    && strcmp(state->map, "ctf3") != 0
	    && strcmp(state->map, "ctf4") != 0
	    && strcmp(state->map, "ctf5") != 0
	    && strcmp(state->map, "ctf6") != 0
	    && strcmp(state->map, "ctf7") != 0
	    && strcmp(state->map, "dm1") != 0
	    && strcmp(state->map, "dm2") != 0
	    && strcmp(state->map, "dm6") != 0
	    && strcmp(state->map, "dm7") != 0
	    && strcmp(state->map, "dm8") != 0
	    && strcmp(state->map, "dm9") != 0)
		return 0;

	if (state->num_clients > MAX_CLIENTS)
		return 0;
	if (state->max_clients > MAX_CLIENTS)
		return 0;

	return 1;
}

static int handle_data(struct data *data, struct netserver *ns)
{
	struct server_state new;
	int rankable;

	assert(data != NULL);
	assert(ns != NULL);

	if (!skip_header(data, MSG_INFO, sizeof(MSG_INFO)))
		return 0;
	if (!unpack_server_state(data, &new))
		return 0;

	rankable = is_vanilla(&new) && strcmp(new.gametype, "CTF") == 0;

	mark_server_online(&new, rankable);
	write_server_state(&new, ns->filename);

	if (rankable) {
		int elapsed = time(NULL) - ns->state.last_seen;
		struct delta delta;

		remove_spectators(&new);
		delta = delta_states(&ns->state, &new, elapsed);
		print_delta(&delta);
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

static int add_netserver(struct netserver_list *list, struct netserver *ns)
{
	static const unsigned OFFSET = 4096;

	if (list->length % OFFSET == 0) {
		struct netserver *tmp;
		size_t newsize = sizeof(*tmp) * (list->length + OFFSET);

		if (!(tmp = realloc(list->netservers, newsize)))
			return 0;

		list->netservers = tmp;
	}

	list->netservers[list->length++] = *ns;
	return 1;
}

static int fill_netserver_list(struct netserver_list *list)
{
	DIR *dir;
	struct dirent *dp;
	char path[PATH_MAX];
	unsigned count = 0;

	assert(list != NULL);

	if (!dbpath(path, PATH_MAX, "servers"))
		return 0;

	dir = opendir(path);
	if (!dir) {
		perror(path);
		return 0;
	}

	/* Fill array (ignore server on error) */
	while ((dp = readdir(dir))) {
		char ip[IP_LENGTH + 1], port[PORT_LENGTH + 1];
		struct netserver ns;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		count++;

		if (read_server_state(&ns.state, dp->d_name) != SUCCESS)
			continue;
		if (!server_expired(&ns.state))
			continue;

		strcpy(ns.filename, dp->d_name);
		if (!extract_ip_and_port(dp->d_name, ip, port))
			continue;
		if (!get_sockaddr(ip, port, &ns.addr))
			continue;
		if (!add_netserver(list, &ns))
			continue;
	}

	closedir(dir);

	verbose("%u servers found, %u will be refreshed\n",
	        count, list->length);

	return 1;
}

static struct netserver *get_netserver(struct pool_entry *entry)
{
	assert(entry != NULL);
	return (struct netserver*)((char*)entry - offsetof(struct netserver, entry));
}

static void poll_servers(struct netserver_list *list, struct sockets *sockets)
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
		add_pool_entry(&pool, &list->netservers[i].entry,
		               &list->netservers[i].addr);

	while ((entry = poll_pool(&pool, &answer)))
		handle_data(&answer, get_netserver(entry));

	while ((entry = foreach_failed_poll(&pool))) {
		struct netserver *ns = get_netserver(entry);
		mark_server_offline(&ns->state);
		write_server_state(&ns->state, ns->filename);
		failed_count++;
	}

	verbose("Polling failed for %u servers\n", failed_count);
}

static const struct netserver_list NETSERVER_LIST_ZERO;

int main(int argc, char **argv)
{
	struct sockets sockets;
	struct netserver_list list = NETSERVER_LIST_ZERO;

	load_config(1);
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!fill_netserver_list(&list))
		return EXIT_FAILURE;

	if (!init_sockets(&sockets))
		return EXIT_FAILURE;
	poll_servers(&list, &sockets);
	close_sockets(&sockets);

	return EXIT_SUCCESS;
}
