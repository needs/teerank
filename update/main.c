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

static bool stop;
static void stop_gracefully(int sig)
{
	stop = false;
}

static struct packet PCK_GETINFO;
static struct packet PCK_GETINFO_64;
static struct packet PCK_GETLIST;

static void create_static_packets(void)
{
	uint8_t MSG_GETINFO[] = {
		255, 255, 255, 255, 'g', 'i', 'e', '3', 0
	};
	uint8_t MSG_GETINFO_64[] = {
		255, 255, 255, 255, 'f', 's', 't', 'd', 0
	};
	uint8_t MSG_GETLIST[] = {
		255, 255, 255, 255, 'r', 'e', 'q', '2'
	};

	create_packet(&PCK_GETINFO, MSG_GETINFO, sizeof(MSG_GETINFO));
	create_packet(&PCK_GETINFO_64, MSG_GETINFO, sizeof(MSG_GETINFO_64));
	create_packet(&PCK_GETLIST, MSG_GETLIST, sizeof(MSG_GETLIST));
}

static time_t expire_in(time_t t, time_t delta)
{
	double fact = ((double)rand() / (double)RAND_MAX);
	time_t min = t - delta;
	time_t max = t + delta;

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
	unsigned i;

	for (i = 0; i < sv->num_clients; i++) {
		struct client *c = &sv->clients[i];
		exec(
			"REPLACE INTO players VALUES(?, ?, ?)",
			"sst", c->name, c->clan, time(NULL)
		);
	}
}

/*
 * Called when we have received every data to be able to rank the
 * server.  New data are stored in client->newserver.
 */
static void server_poll_success(struct netclient *client)
{
	struct server *old, *new;

	old = &client->server;
	new = &client->newserver;

	new->lastseen = time(NULL);
	new->expire = expire_in(5 * 60, 30);

	/*
	 * Update players before ranking them so that new players are
	 * created.
	 */
	update_players(new);
	rank_players(old, new);

	write_server(new);
	write_server_clients(new);

	schedule(&client->update, new->expire);

	/* New state becomes the "old" state */
	*old = *new;
}

/*
 * Called when somehow we didn't get all the data to be able to rank the
 * server.  Unpolled entries have their expiry date doubled.
 */
static void server_poll_failure(struct netclient *client)
{
	struct server *server;
	long elapsed_days;

	server = &client->server;
	elapsed_days = (time(NULL) - server->lastseen) / (3600 * 24);

	if (elapsed_days >= 1) {
		remove_server(server->ip, server->port);
		remove_netclient(client);
		return;
	}

	if (client->pentry.polled) {
		server->lastseen = time(NULL);
		server->expire = expire_in(5 * 60, 30);
	} else {
		server->expire = double_expiry_date(server->expire, server->lastseen);
	}

	write_server(server);
	schedule(&client->update, server->expire);
}

static void handle_server_packet(struct netclient *client, struct packet *packet)
{
	enum unpack_status status;
	status = unpack_server_info(packet, &client->newserver);

	switch (status) {
	case UNPACK_ERROR:
		remove_pool_entry(&client->pentry);
		server_poll_failure(client);
		break;

	case UNPACK_SUCCESS:
		remove_pool_entry(&client->pentry);
		server_poll_success(client);
		break;

	case UNPACK_INCOMPLETE:
		/* Just wait for more packets to come */
		break;
	}
}

static void handle_server_timeout(struct netclient *client)
{
	/*
	 * In any cases, we should be able to detect when no more
	 * packets are required.  So when an entry timeout, it is always
	 * a failure.
	 */
	server_poll_failure(client);
}

/* Load netclients and schedule them right away */
static void load_netclients(void)
{
	struct server server;
	struct master master;
	struct netclient *client;
	const char *query;

	sqlite3_stmt *res;

	query =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers";

	foreach_row(res, query) {
		read_server(res, &server);
		read_server_clients(&server);
		if ((client = add_netclient(NETCLIENT_TYPE_SERVER, &server)))
			schedule(&client->update, server.expire);
	}

	query =
		"SELECT node, service, lastseen, expire"
		" FROM masters";

	foreach_row(res, query) {
		read_master(res, &master);
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
	const char *query =
		"SELECT COUNT(1)"
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	if (count_rows(query, "ss", ip, port) == 0) {
		struct server server;
		struct netclient *client;

		server = create_server(ip, port, master->node, master->service);
		client = add_netclient(NETCLIENT_TYPE_SERVER, &server);
		if (client)
			schedule(&client->update, 0);

		return;
	}

	exec(
		"INSERT OR IGNORE INTO server_masters"
		" VALUES(?, ?, ?, ?)",
		"ssss", ip, port, master->node, master->service
	);
}

static void handle_master_packet(struct netclient *client, struct packet *packet)
{
	char *ip, *port;

	assert(client != NULL);
	assert(packet != NULL);

	while (unpack_server_addr(packet, &ip, &port) == UNPACK_INCOMPLETE)
		reference_server(ip, port, &client->master);
}

/*
 * Eventually every masters will timeout because we never remove them
 * from the pool.  What matter is if whether or not they did send data
 * before going quiet.
 */
static void handle_master_timeout(struct netclient *client)
{
	struct master *master = &client->master;

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
	exec(
		"DELETE FROM server_masters"
		" WHERE master_node = ? AND master_service = ?",
		"ss", master->node, master->service
	);
}

static void add_to_pool(struct netclient *client)
{
	const struct packet *request = NULL;

	switch (client->type) {
	case NETCLIENT_TYPE_SERVER:
		client->newserver = (struct server){ 0 };
		strcpy(client->newserver.ip, client->server.ip);
		strcpy(client->newserver.port, client->server.port);

		if (is_legacy_64(&client->server))
			request = &PCK_GETINFO_64;
		else
			request = &PCK_GETINFO;
		break;

	case NETCLIENT_TYPE_MASTER:
		unreference_servers(&client->master);
		request = &PCK_GETLIST;
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

	struct job recompute_ranks_job;
	bool do_recompute_ranks = false;

	if (!have_schedule())
		return EXIT_SUCCESS;
	if (!init_network())
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

		while ((pentry = poll_pool(&packet)))
			handle(get_netclient(pentry, pentry), packet);

		if (do_recompute_ranks)
			recompute_ranks();

		if (dberr) {
			exec("ROLLBACK");
			return EXIT_FAILURE;
		}
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

	create_static_packets();
	load_netclients();
	return update();
}
