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
#include <stdbool.h>

#include <netinet/in.h>

#include "pool.h"
#include "teerank.h"
#include "server.h"
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

/* Update clan and lastseen date of connected clients */
static void update_players(struct server *sv)
{
	struct client *c;
	unsigned i, nrow;
	struct sqlite3_stmt *res;

	const char *exist =
		"SELECT null FROM players WHERE name = ?";

	const char *update =
		"UPDATE players"
		" SET clan = ?, lastseen = ?, server_ip = ?, server_port = ?"
		" WHERE name = ?";

	const char *insert =
		"INSERT INTO players"
		" VALUES(?, ?, ?, ?, ?)";

	for (i = 0; i < sv->num_clients; i++) {
		c = &sv->clients[i];

		foreach_row(exist, NULL, NULL, "s", c->name);

		if (res && nrow)
			exec(update, "stsss", c->clan, time(NULL), sv->ip, sv->port, c->name);
		else if (res)
			exec(insert, "sstss", c->name, c->clan, time(NULL), sv->ip, sv->port);
	}
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
		/* Update players before ranking them so that new
		 * players are created */
		update_players(new);
		rank_players(&old, new);
		write_server_clients(new);
	}

	new->expire = expire_in(5 * 60, 30);

	write_server(new);
	schedule(&client->update, new->expire);
}

static void handle_server_timeout(struct netclient *client)
{
	struct server *server = &client->info.server;
	long elapsed_days = (time(NULL) - server->lastseen) / (3600 * 24);

	if (elapsed_days >= 1) {
		remove_server(server->ip, server->port);
		remove_netclient(client);
		return;
	}

	server->expire = double_expiry_date(server->expire, server->lastseen);
	schedule(&client->update, server->expire);
	write_server(server);
}

/* Load netclients and schedule them right away */
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

	foreach_row(query, read_server, &server) {
		read_server_clients(&server);
		if ((client = add_netclient(NETCLIENT_TYPE_SERVER, &server)))
			schedule(&client->update, server.expire);
	}

	query =
		"SELECT node, service, lastseen, expire"
		" FROM masters";

	foreach_row(query, read_master, &master) {
		if ((client = add_netclient(NETCLIENT_TYPE_MASTER, &master)))
			schedule(&client->update, master.expire);
	}
}

/*
 * Set master node and service on the given server.  If the server
 * doesn't exist, create it and schedule it.
 */
static void reference_server(char *ip, char *port, struct master *master)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct server s;

	const char *query =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	foreach_row(query, read_server, &s, "ss", ip, port);

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
 * Don't do anything if there are no jobs scheduled.  Once in the main
 * loop, stops when 'stop' is set.
 */
static int update(void)
{
	struct pool_entry *pentry;
	struct packet *packet;
	struct job *job;

	struct sockets sockets;

	struct job recompute_ranks_job;
	bool do_recompute_ranks = false;

	if (!have_schedule())
		return EXIT_SUCCESS;
	if (!init_sockets(&sockets))
		return EXIT_FAILURE;

	/*
	 * Schedule rank recomputing at the start of the program so that
	 * brand new databases quickly have a player list with ranks.
	 * Schedule it after a little delay to let finish masters and
	 * servers polling.
	 */
	schedule(&recompute_ranks_job, expire_in(10, 0));

	while (!stop) {
		wait_until_next_schedule();

		while ((job = next_schedule())) {
			if (job == &recompute_ranks_job)
				do_recompute_ranks = true;
			else
				add_to_pool(get_netclient(job, update));
		}

		/*
		 * Batch database queries because in the worst case,
		 * every servers will be polled.  Doing more than 1000
		 * individual transaction is very slow.
		 */
		exec("BEGIN");

		while ((pentry = poll_pool(&sockets, &packet)))
			handle(get_netclient(pentry, pentry), packet);

		if (do_recompute_ranks)
			recompute_ranks();

		exec("COMMIT");

		/*
		 * Force a WAL checkpoint after recomputing ranks
		 * because a lot of data are written in the wal wfile
		 * and we don't want it to grow without bounds.
		 */
		if (do_recompute_ranks) {
			exec("PRAGMA wal_checkpoint");
			schedule(&recompute_ranks_job, expire_in(5 * 60, 0));
			do_recompute_ranks = false;
		}
	}

	close_sockets(&sockets);
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	init_teerank(0);

	signal(SIGINT,  stop_gracefully);
	signal(SIGTERM, stop_gracefully);

	load_netclients();
	return update();
}
