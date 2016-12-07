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

#include "pool.h"
#include "config.h"
#include "server.h"
#include "player.h"
#include "scheduler.h"
#include "netclient.h"
#include "rank.h"
#include "packet.h"
#include "unpacker.h"

static int stop;
static void stop_gracefully(int sig)
{
	stop = 1;
}

static const struct packet MSG_GETINFO = {
	9, {
		255, 255, 255, 255, 'g', 'i', 'e', '3', 0
	}
};
static const struct packet MSG_GETLIST = {
	8, {
		255, 255, 255, 255, 'r', 'e', 'q', '2'
	}
};

static time_t expire_in(time_t sec, time_t maxdist)
{
	double fact = ((double)rand() / (double)RAND_MAX);
	time_t min = sec - maxdist;
	time_t max = sec + maxdist;

	return time(NULL) + min + (max - min) * fact;
}

/*
 * We won't want to check an offline server too often, because it will
 * add a (probably) unnecessary timeout delay when polling.  However
 * sometime the server is online but our UDP packets got lost 3 times in
 * a row, in this case we don't want to delay too much the next poll.
 *
 * So doubling the expiry date seems to be adequate.  If the server was
 * seen 5 minutes ago, the next poll will be scheduled in 10 minutes,
 * and so on up to a maximum of 2 hours.
 */
static time_t double_expiry_date(time_t lastexpire, time_t lastseen)
{
	time_t t;

	if (lastexpire < lastseen)
		t = 5 * 60;
	else
		t = lastexpire - lastseen;

	if (t > 2 * 3600)
		t = 2 * 3600;
	else if (t < 5 * 60)
		t = 5 * 60;

	return expire_in(t, 0);
}

static void handle_server_packet(struct netclient *client, struct packet *packet)
{
	struct server old, *new;

	assert(client != NULL);
	assert(packet != NULL);

	/* In any cases, we expect only one answer */
	remove_pool_entry(&client->pentry);

	old = client->info.server;
	new = &client->info.server;
	new->lastseen = time(NULL);

	if (unpack_server_info(packet, new)) {
		rank_players(&old, new);
		write_server_clients(new);
	}

	if (is_vanilla_ctf(new->gametype, new->map, new->max_clients))
		new->expire = expire_in(5 * 60, 60);
	else
		new->expire = expire_in(3600, 5 * 60);

	write_server(new);
	schedule(&client->update, new->expire);
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

	server->expire = double_expiry_date(server->expire, server->lastseen);
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

static void reference_server(char *ip, char *port, struct master *master)
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

static void handle_master_packet(struct netclient *client, struct packet *packet)
{
	char *ip, *port;
	int reset_context = 1;

	assert(client != NULL);
	assert(packet != NULL);

	while (unpack_server_addr(packet, &ip, &port, &reset_context))
		reference_server(ip, port, &client->info.master);
}

/*
 * Eventually every masters will timeout because we never remove them
 * from the pool.  What matter is if whether or not they did send data
 * before going quiet.
 */
static void handle_master_timeout(struct netclient *client)
{
	struct master *master = &client->info.master;

	if (client->pentry.polled) { /* Online */
		master->expire = expire_in(5 * 60, 1 * 60);
		master->lastseen = time(NULL);

	} else { /* Offline */
		master->expire = double_expiry_date(master->expire, master->lastseen);
	}

	write_master(master);
	schedule(&client->update, master->expire);
}

static void handle(struct netclient *client, struct packet *packet)
{
	switch (client->type) {
	case NETCLIENT_TYPE_SERVER:
		if (packet)
			handle_server_packet(client, packet);
		else
			handle_server_timeout(client);
		break;

	case NETCLIENT_TYPE_MASTER:
		if (packet)
			handle_master_packet(client, packet);
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
	const struct packet *request = NULL;

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
	struct packet *packet;
	struct job *job;

	assert(sockets != NULL);

	while (!stop && have_schedule()) {
		wait_until_next_schedule();

		while ((job = next_schedule()))
			add_to_pool(get_netclient(job, update));

		/*
		 * Batch database queries because in the worst case,
		 * every servers will be polled.  Doing more than 1000
		 * individual transaction is very slow.
		 */
		exec("BEGIN");
		while ((pentry = poll_pool(sockets, &packet)))
			handle(get_netclient(pentry, pentry), packet);
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
