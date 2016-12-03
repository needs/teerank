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

static int stop;
static void stop_gracefully(int sig)
{
	stop = 1;
}

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

struct netserver {
	struct sockaddr_storage addr;
	struct server server;
	struct pool_entry entry;
	struct job update;
};

static struct netserver *servers;
static unsigned nr_servers;

static int handle_data(struct pool *pool, struct data *data, struct netserver *ns)
{
	struct server old;
	struct delta delta;
	int elapsed;

	assert(data != NULL);
	assert(ns != NULL);

	/* In any cases, we expect only one answer */
	remove_pool_entry(pool, &ns->entry);
	old = ns->server;
	mark_server_online(&ns->server);

	if (!skip_header(data, MSG_INFO, sizeof(MSG_INFO)))
		return 1;
	if (!unpack_packet_data(data, &ns->server))
		return 1;

	/*
	 * A new server have an elapsed time set to zero, so it wont be
	 * ranked.  If somehow it still try to be ranked, every players
	 * will have NO_SCORE in the "old_score" field of the delta,
, no players will be ranked anyway.
	 */
	if (old.lastseen == NEVER_SEEN)
		elapsed = 0;
	else
		elapsed = time(NULL) - old.lastseen;

	delta = delta_servers(&old, &ns->server, elapsed);
	print_delta(&delta);

	return 1;
}

static long elapsed_days(time_t t)
{
	if (t == NEVER_SEEN)
		return 0;

	return (time(NULL) - t) / (3600 * 24);
}

static int handle_no_data(struct netserver *ns)
{
	if (elapsed_days(ns->server.lastseen) >= 1) {
		remove_server(ns->server.ip, ns->server.port);
		return 0;
	}

	mark_server_offline(&ns->server);
	return 1;
}

static struct netserver *new_netserver(void)
{
	static const unsigned STEP = 4096;

	if (nr_servers % STEP == 0) {
		struct netserver *tmp;
		size_t newsize = sizeof(*tmp) * (nr_servers + STEP);

		if (!(tmp = realloc(servers, newsize)))
			return NULL;

		servers = tmp;
	}

	return &servers[nr_servers++];
}

static void trash_newest_netserver(void)
{
	assert(nr_servers > 0);
	nr_servers--;
}

static void add_netserver(struct server *server)
{
	struct netserver *ns = new_netserver();

	if (!ns)
		return;
	if (!read_server_clients(server))
		goto fail;
	if (!get_sockaddr(server->ip, server->port, &ns->addr))
		goto fail;

	ns->server = *server;
	return;

fail:
	trash_newest_netserver();
}

static void load_servers(void)
{
	struct server server;
	sqlite3_stmt *res;
	unsigned nrow;

	const char *query =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers";

	foreach_server(query, &server)
		add_netserver(&server);

	verbose("%u servers loaded (over %u)\n", nr_servers, nrow);
}

#define get_netserver(ptr, field) \
	(void*)((char*)ptr - offsetof(struct netserver, field))

/*
 * This does only stops when 'stop' is set, by a signal.
 */
static void poll_servers(struct sockets *sockets)
{
	struct pool pool;
	struct pool_entry *pentry;
	struct data *data;
	struct netserver *ns;
	struct job *job;
	unsigned i;
	int reschedule;

	const struct data REQUEST = {
		sizeof(MSG_GETINFO) + 1, {
			MSG_GETINFO[0], MSG_GETINFO[1], MSG_GETINFO[2],
			MSG_GETINFO[3], MSG_GETINFO[4], MSG_GETINFO[5],
			MSG_GETINFO[6], MSG_GETINFO[7], 0
		}
	};

	assert(sockets != NULL);

	init_pool(&pool, sockets);

	for (i = 0; i < nr_servers; i++)
		schedule(&servers[i].update, servers[i].server.expire);

	while (!stop && nr_servers > 0) {
		sleep(waiting_time());

		while ((job = next_schedule())) {
			ns = get_netserver(job, update);
			add_pool_entry(&pool, &ns->entry, &ns->addr, &REQUEST);
		}

		/*
		 * Batch database queries because in the worst case,
		 * every servers will be polled.  Doing more than 1000
		 * individual transaction is very slow.
		 */
		exec("BEGIN");
		while ((pentry = poll_pool(&pool, &data))) {
			ns = get_netserver(pentry, entry);

			if (data)
				reschedule = handle_data(&pool, data, ns);
			else
				reschedule = handle_no_data(ns);

			write_server(&ns->server);
			write_server_clients(&ns->server);

			if (reschedule)
				schedule(&ns->update, ns->server.expire);
		}
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
	load_servers();

	poll_servers(&sockets);
	close_sockets(&sockets);

	return EXIT_SUCCESS;
}
