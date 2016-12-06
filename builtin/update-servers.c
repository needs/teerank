#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include <netinet/in.h>

#include "network.h"
#include "pool.h"
#include "delta.h"
#include "config.h"
#include "server.h"
#include "player.h"
#include "scheduler.h"
#include "netclient.h"

static int stop;
static void stop_gracefully(int sig)
{
	stop = 1;
}

static const struct data MSG_GETINFO = {
	9, {
		255, 255, 255, 255, 'g', 'i', 'e', '3', 0
	}
};
static const uint8_t MSG_INFO[] = {
	255, 255, 255, 255, 'i', 'n', 'f', '3'
};

static const struct data MSG_GETLIST = {
	8, {
		255, 255, 255, 255, 'r', 'e', 'q', '2'
	}
};
static const uint8_t MSG_LIST[] = {
	255, 255, 255, 255, 'l', 'i', 's', '2'
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

static int unpack_packet_data(struct data *data, struct server *sv)
{
	struct unpacker up;
	unsigned i;

	assert(data != NULL);
	assert(sv != NULL);

	init_unpacker(&up, data);

	if (!can_unpack(&up, 10))
		return 0;

	skip_field(&up); /* Token */
	skip_field(&up); /* Version */
	unpack_string(&up, sv->name,     sizeof(sv->name));     /* Name */
	unpack_string(&up, sv->map,      sizeof(sv->map));      /* Map */
	unpack_string(&up, sv->gametype, sizeof(sv->gametype)); /* Gametype */

	skip_field(&up); /* Flags */
	skip_field(&up); /* Player number */
	skip_field(&up); /* Player max number */
	sv->num_clients = unpack_int(&up); /* Client number */
	sv->max_clients = unpack_int(&up); /* Client max number */

	if (sv->num_clients > MAX_CLIENTS)
		return 0;
	if (sv->max_clients > MAX_CLIENTS)
		return 0;
	if (sv->num_clients > sv->max_clients)
		return 0;

	/* Clients */
	for (i = 0; i < sv->num_clients; i++) {
		struct client *cl = &sv->clients[i];

		if (!can_unpack(&up, 5))
			return 0;

		unpack_string(&up, cl->name, sizeof(cl->name)); /* Name */
		unpack_string(&up, cl->clan, sizeof(cl->clan)); /* Clan */

		skip_field(&up); /* Country */
		cl->score  = unpack_int(&up); /* Score */
		cl->ingame = unpack_int(&up); /* Ingame? */
	}

	return 1;
}

static void handle_server_data(struct netclient *client, struct data *data)
{
	struct server old, *server;
	struct delta delta;
	int elapsed;

	assert(client != NULL);
	assert(data != NULL);

	/* In any cases, we expect only one answer */
	remove_pool_entry(&client->pentry);

	server = &client->info.server;
	old = *server;
	mark_server_online(server);

	if (!skip_header(data, MSG_INFO, sizeof(MSG_INFO)))
		goto out;
	if (!unpack_packet_data(data, server))
		goto out;

	/*
	 * A new server have an elapsed time set to zero, so it wont be
	 * ranked.  If somehow it still try to be ranked, every players
	 * will have NO_SCORE in the "old_score" field of the delta,
	 * no players will be ranked anyway.
	 */
	if (old.lastseen == NEVER_SEEN)
		elapsed = 0;
	else
		elapsed = time(NULL) - old.lastseen;

	delta = delta_servers(&old, server, elapsed);
	print_delta(&delta);

	write_server_clients(server);

out:
	write_server(server);
	schedule(&client->update, server->expire);
}

static long elapsed_days(time_t t)
{
	if (t == NEVER_SEEN)
		return 0;

	return (time(NULL) - t) / (3600 * 24);
}

static void handle_server_timeout(struct netclient *client)
{
	struct server *server = &client->info.server;

	if (elapsed_days(server->lastseen) >= 1) {
		remove_server(server->ip, server->port);
		remove_netclient(client);
		return;
	}

	mark_server_offline(server);
	schedule(&client->update, server->expire);
	write_server(server);
}

static void load_netclients(void)
{
	struct server server;
	struct master master;
	struct netclient *client;
	const char *query;

	sqlite3_stmt *res;
	unsigned nrow;

	query =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers";

	foreach_server(query, &server) {
		read_server_clients(&server);
		if ((client = add_netclient(NETCLIENT_TYPE_SERVER, &server)))
			schedule(&client->update, server.expire);
	}

	query =
		"SELECT" ALL_MASTER_COLUMNS
		" FROM masters";

	foreach_master(query, &master) {
		if ((client = add_netclient(NETCLIENT_TYPE_MASTER, &master)))
			schedule(&client->update, master.expire);
	}
}

static void update_server(char *ip, char *port, struct master *master)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct server s;

	const char *query =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	foreach_server(query, &s, "ss", ip, port);

	if (!res)
		return;

	if (!nrow) {
		struct server server;
		struct netclient *client;

		server = create_server(ip, port, master->node, master->service);
		client = add_netclient(NETCLIENT_TYPE_SERVER, &server);
		if (client)
			schedule(&client->update, 0);

		return;
	}

	query =
		"UPDATE servers"
		" SET master_node = ?, master_service = ?"
		" WHERE ip = ? AND port = ?";

	exec(query, "ssss", master->node, master->service, ip, port);
}

struct server_addr_raw {
	uint8_t ip[16];
	uint8_t port[2];
};

static int is_ipv4(unsigned char *ip)
{
	static const unsigned char IPV4_HEADER[] = {
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0xFF
	};

	assert(ip != NULL);

	return memcmp(ip, IPV4_HEADER, sizeof(IPV4_HEADER)) == 0;
}

static void raw_addr_to_strings(
	struct server_addr_raw *raw, char **_ip, char **_port)
{
	char ip[IP_STRSIZE], port[PORT_STRSIZE];
	uint16_t portnum;
	int ret;

	assert(raw != NULL);
	assert(_ip != NULL);
	assert(_port != NULL);

	/* Convert raw adress to either ipv4 or ipv6 string format */
	if (is_ipv4(raw->ip)) {
		ret = snprintf(
			ip, sizeof(ip), "%u.%u.%u.%u",
			raw->ip[12], raw->ip[13], raw->ip[14], raw->ip[15]);
	} else {
		ret = snprintf(
			ip, sizeof(ip),
		        "%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x",
		        raw->ip[0], raw->ip[1], raw->ip[2], raw->ip[3],
		        raw->ip[4], raw->ip[5], raw->ip[6], raw->ip[7],
		        raw->ip[8], raw->ip[9], raw->ip[10], raw->ip[11],
		        raw->ip[12], raw->ip[13], raw->ip[14], raw->ip[15]);
	}

	assert(ret > 0 && ret < sizeof(ip));

	/* Unpack port and then write it in a string */
	portnum = (raw->port[0] << 8) | raw->port[1];
	ret = snprintf(port, sizeof(port), "%u", portnum);

	assert(ret > 0 && ret < sizeof(port));
	(void)ret; /* Don't raise a warning when assertions are disabled */

	*_ip = ip;
	*_port = port;
}

static void handle_master_data(struct netclient *client, struct data *data)
{
	unsigned char *buf;
	int size;

	assert(client != NULL);
	assert(data != NULL);

	if (!skip_header(data, MSG_LIST, sizeof(MSG_LIST)))
		return;

	size = data->size;
	buf = data->buffer;

	while (size >= sizeof(struct server_addr_raw)) {
		struct server_addr_raw *raw = (struct server_addr_raw*)buf;
		char *ip, *port;

		raw_addr_to_strings(raw, &ip, &port);
		update_server(ip, port, &client->info.master);

		buf += sizeof(*raw);
		size -= sizeof(*raw);
	}
}

static time_t expire_in(time_t sec, time_t maxdist)
{
	double fact = ((double)rand() / (double)RAND_MAX);
	time_t min = sec - maxdist;
	time_t max = sec + maxdist;

	return time(NULL) + min + (max - min) * fact;
}

/*
 * Eventually every masters will timeout because we never remove them
 * from the pool.  What matter is if whether or not they did sent data
 * before going quiet.
 */
static void handle_master_timeout(struct netclient *client)
{
	struct master *master = &client->info.master;

	if (client->pentry.polled) { /* Online */
		master->expire = expire_in(5 * 60, 1 * 60);
		master->lastseen = time(NULL);

	} else { /* Offline */
		time_t t = master->expire - master->lastseen;

		if (t > 2 * 3600)
			t = 2 * 3600;

		master->expire = expire_in(t, 0);
	}

	write_master(master);
	schedule(&client->update, master->expire);
}

static void handle(struct netclient *client, struct data *data)
{
	switch (client->type) {
	case NETCLIENT_TYPE_SERVER:
		if (data)
			handle_server_data(client, data);
		else
			handle_server_timeout(client);
		break;

	case NETCLIENT_TYPE_MASTER:
		if (data)
			handle_master_data(client, data);
		else
			handle_master_timeout(client);
		break;
	}
}

/*
 * Called before polling the given master, so that server not referenced
 * anymore don't keep a dangling reference to their old master.
 */
static void unreference_servers(struct master *master)
{
	const char *query =
		"UPDATE servers"
		" SET master_node = '', master_service = ''"
		" WHERE master_node = ? AND master_service = ?";

	exec(query, "ss", master->node, master->service);
}

static void add_to_pool(struct netclient *client)
{
	const struct data *request = NULL;

	switch (client->type) {
	case NETCLIENT_TYPE_SERVER:
		request = &MSG_GETINFO;
		break;
	case NETCLIENT_TYPE_MASTER:
		unreference_servers(&client->info.master);
		request = &MSG_GETLIST;
		break;
	}

	add_pool_entry(&client->pentry, &client->addr, request);
}

#define get_netclient(ptr, field) \
	(void*)((char*)ptr - offsetof(struct netclient, field))

/*
 * It stops when 'stop' is set (by a signal) or when there is no job
 * scheduled.
 */
static void poll_servers(struct sockets *sockets)
{
	struct pool_entry *pentry;
	struct data *data;
	struct job *job;

	assert(sockets != NULL);

	while (!stop && have_schedule()) {
		sleep(waiting_time());

		while ((job = next_schedule()))
			add_to_pool(get_netclient(job, update));

		/*
		 * Batch database queries because in the worst case,
		 * every servers will be polled.  Doing more than 1000
		 * individual transaction is very slow.
		 */
		exec("BEGIN");
		while ((pentry = poll_pool(sockets, &data)))
			handle(get_netclient(pentry, pentry), data);
		exec("COMMIT");
	}
}

int main(int argc, char **argv)
{
	struct sockets sockets;

	load_config(1);

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	signal(SIGINT,  stop_gracefully);
	signal(SIGTERM, stop_gracefully);

	if (!init_sockets(&sockets))
		return EXIT_FAILURE;
	load_netclients();

	poll_servers(&sockets);
	close_sockets(&sockets);

	return EXIT_SUCCESS;
}
