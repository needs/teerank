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
#include "config.h"
#include "server.h"
#include "player.h"
#include "scheduler.h"
#include "netclient.h"
#include "elo.h"

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

/*
 * Given a game it does return how many players are rankable.
 */
static unsigned mark_rankable_players(
	struct delta *delta, struct player *players, unsigned length)
{
	unsigned i, rankable = 0;

	assert(players != NULL);

	/*
	 * 30 minutes between each update is just too much and it increase
	 * the chance of rating two different games.
	 */
	if (delta->elapsed > 30 * 60)
		return 0;

	/*
	 * On the other hand, less than 1 minutes between updates is
	 * also meaningless.
	 */
	if (delta->elapsed < 60)
		return 0;

	if (!is_vanilla_ctf(delta->gametype, delta->map, delta->max_clients))
		return 0;


	/* Mark rankable players */
	for (i = 0; i < length; i++) {
		if (players[i].delta->ingame) {
			players[i].is_rankable = 1;
			rankable++;
		}
	}

	/*
	 * We don't rank games with less than 4 rankable players.  We believe
	 * it is too much volatile to rank those kind of games.
	 */
	if (rankable < 4)
		return 0;

	verbose(
		"%s, %u rankable players over %u\n",
		build_addr(delta->ip, delta->port), rankable, length);

	return rankable;
}

static void merge_delta(struct player *player, struct player_delta *delta)
{
	assert(player != NULL);
	assert(delta != NULL);

	player->delta = delta;

	/*
	 * Store the old clan to be able to write a delta once
	 * write_player() succeed.
	 */
	if (strcmp(player->clan, delta->clan) != 0) {
		char tmp[NAME_LENGTH];
		/* Put the old clan in the delta so we can use it later */
		strcpy(tmp, player->clan);
		set_clan(player, delta->clan);
		strcpy(delta->clan, tmp);
	}

	player->is_rankable = 0;
}

static void read_rank(sqlite3_stmt *res, void *_rank)
{
	unsigned *rank = _rank;
	*rank = sqlite3_column_int64(res, 0);
}

static void compute_ranks(struct player *players, unsigned len)
{
	unsigned i, nrow;
	sqlite3_stmt *res;
	struct player *p;

	const char query[] =
		"SELECT" RANK_COLUMN
		" FROM players"
		" WHERE name = ?";

	for (i = 0; i < len; i++) {
		p = &players[i];

		if (!p->is_rankable)
			continue;

		foreach_row(query, read_rank, &p->rank, "s", p->name);
		record_elo_and_rank(p);
	}
}

static int load_player(struct player *p, const char *pname)
{
	unsigned nrow;
	sqlite3_stmt *res;

	const char query[] =
		"SELECT" ALL_PLAYER_COLUMNS
		" FROM players"
		" WHERE name = ?";

	foreach_player(query, p, "s", pname);

	if (!res)
		return 0;
	if (!nrow)
		create_player(p, pname);

	return 1;
}

static void process_delta(struct delta *delta)
{
	struct player players[MAX_CLIENTS];
	unsigned i, length = 0;
	int rankedgame;

	/* Load player (ignore fail) */
	for (i = 0; i < delta->length; i++) {
		struct player *player;
		const char *name;

		player = &players[length];
		name = delta->players[i].name;

		if (load_player(player, name)) {
			merge_delta(player, &delta->players[i]);
			length++;
		}
	}

	rankedgame = mark_rankable_players(delta, players, length);

	if (rankedgame)
		compute_elos(players, length);

	for (i = 0; i < length; i++) {
		set_lastseen(&players[i], delta->ip, delta->port);
		write_player(&players[i]);
	}

	/*
	 * We need player to have been written to the database
	 * before calculating rank, because such calculation
	 * only use what's in the database.
	 */
	if (rankedgame)
		compute_ranks(players, length);
}


static struct client *get_player(
	struct server *server, struct client *client)
{
	unsigned i;

	assert(server != NULL);
	assert(client != NULL);

	for (i = 0; i < server->num_clients; i++)
		if (strcmp(server->clients[i].name, client->name) == 0)
			return &server->clients[i];

	return NULL;
}

static void delta_servers(
	struct delta *delta, struct server *old, struct server *new, int elapsed)
{
	unsigned i;

	assert(old != NULL);
	assert(new != NULL);

	strcpy(delta->ip, new->ip);
	strcpy(delta->port, new->port);

	strcpy(delta->gametype, new->gametype);
	strcpy(delta->map, new->map);

	delta->num_clients = new->num_clients;
	delta->max_clients = new->max_clients;

	delta->elapsed = elapsed;
	delta->length = new->num_clients;

	for (i = 0; i < new->num_clients; i++) {
		struct client *old_player, *new_player;
		struct player_delta *player;

		player = &delta->players[i];
		new_player = &new->clients[i];
		old_player = get_player(old, new_player);

		strcpy(player->name, new_player->name);
		strcpy(player->clan, new_player->clan);
		player->score = new_player->score;
		player->ingame = new_player->ingame;

		if (old_player)
			player->old_score = old_player->score;
		else
			player->old_score = NO_SCORE;
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

	delta_servers(&delta, &old, server, elapsed);
	process_delta(&delta);

	write_server_clients(server);

out:
	if (is_vanilla_ctf(server->gametype, server->map, server->max_clients))
		server->expire = expire_in(5 * 60, 60);
	else
		server->expire = expire_in(3600, 5 * 60);

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
		master->expire = double_expiry_date(master->expire, master->lastseen);
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
