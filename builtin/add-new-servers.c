#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <limits.h>

#include "config.h"
#include "network.h"
#include "server.h"
#include "pool.h"
#include "master.h"

struct master_info {
	struct master info;
	short is_online;

	struct pool_entry pentry;
	struct sockaddr_storage addr;
};

/*
 * Add one extra element so that we can iterate the array and stop when
 * node is empty, removing the need for keeping count of masters.
 */
static struct master_info masters[MAX_MASTERS + 1];

static int init_masters_list(void)
{
	unsigned nrow;
	sqlite3_stmt *res;

	const char query[] =
		"SELECT" ALL_MASTER_COLUMNS
		" FROM masters";

	foreach_master(query, &masters[nrow])
		if (nrow == MAX_MASTERS)
			break_foreach;

	if (res)
		return 1;

	return 0;
}

static const uint8_t MSG_GETLIST[] = {
	255, 255, 255, 255, 'r', 'e', 'q', '2'
};
static const uint8_t MSG_LIST[] = {
	255, 255, 255, 255, 'l', 'i', 's', '2'
};

struct entry {
	struct server_addr addr;
	struct master_info *master;
};

struct list {
	unsigned length;
	struct entry *entries;
};

static void add(struct list *list, struct server_addr *addr, struct master_info *master)
{
	const unsigned OFFSET = 1024;

	assert(list != NULL);
	assert(addr != NULL);

	if (list->length % OFFSET == 0) {
		list->entries = realloc(
			list->entries,
			(list->length + OFFSET) * sizeof(*list->entries));
		if (!list->entries) {
			perror("realloc(list)");
			exit(EXIT_FAILURE);
		}
	}

	list->entries[list->length].addr = *addr;
	list->entries[list->length].master = master;
	list->length++;
}

static int is_ipv4(unsigned char *ip)
{
	static const unsigned char ipv4_header[] = {
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0xFF
	};

	assert(ip != NULL);

	return memcmp(ip, ipv4_header, sizeof(ipv4_header)) == 0;
}

struct server_addr_raw {
	uint8_t ip[16];
	uint8_t port[2];
};

static void raw_addr_to_addr(
	struct server_addr_raw *raw, struct server_addr *addr)
{
	uint16_t port;
	int ret;

	assert(raw != NULL);
	assert(addr != NULL);

	/* Convert raw adress to either ipv4 or ipv6 string format */
	if (is_ipv4(raw->ip)) {
		addr->version = IPV4;
		ret = snprintf(
			addr->ip, sizeof(addr->ip), "%u.%u.%u.%u",
			raw->ip[12], raw->ip[13], raw->ip[14], raw->ip[15]);
	} else {
		addr->version = IPV6;
		ret = snprintf(
			addr->ip, sizeof(addr->ip),
		        "%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x",
		        raw->ip[0], raw->ip[1], raw->ip[2], raw->ip[3],
		        raw->ip[4], raw->ip[5], raw->ip[6], raw->ip[7],
		        raw->ip[8], raw->ip[9], raw->ip[10], raw->ip[11],
		        raw->ip[12], raw->ip[13], raw->ip[14], raw->ip[15]);
	}

	assert(ret > 0 && ret < sizeof(addr->ip));

	/* Unpack port and then write it in a string */
	port = (raw->port[0] << 8) | raw->port[1];
	ret = snprintf(addr->port, sizeof(addr->port), "%u", port);

	assert(ret > 0 && ret < sizeof(addr->port));

	/* Don't raise a warning when assertions are disabled */
	(void)ret;
}

static void handle_data(struct data *data, struct list *list, struct master_info *master)
{
	unsigned char *buf;
	unsigned added = 0;
	int size;

	assert(data != NULL);
	assert(list != NULL);
	assert(master != NULL);

	if (!skip_header(data, MSG_LIST, sizeof(MSG_LIST)))
		return;

	size = data->size;
	buf = data->buffer;

	while (size >= sizeof(struct server_addr_raw)) {
		struct server_addr_raw *raw = (struct server_addr_raw*)buf;
		struct server_addr addr;

		raw_addr_to_addr(raw, &addr);
		add(list, &addr, master);
		added++;

		buf += sizeof(*raw);
		size -= sizeof(*raw);
	}

	master->is_online = 1;
}

static struct master_info *get_master(struct pool_entry *entry)
{
	assert(entry != NULL);
	return (struct master_info*)((char*)entry - offsetof(struct master_info, pentry));
}

static void fill_list(struct list *list)
{
	struct pool pool;
	struct pool_entry *entry;
	struct sockets sockets;
	struct data request, *data;
	struct master_info *m;

	assert(list != NULL);

	list->length = 0;
	list->entries = NULL;

	if (!init_sockets(&sockets))
		exit(EXIT_FAILURE);

	request.size = sizeof(MSG_GETLIST);
	memcpy(request.buffer, MSG_GETLIST, request.size);
	init_pool(&pool, &sockets);

	for (m = masters; m->info.node[0]; m++) {
		if (!get_sockaddr(m->info.node, m->info.service, &m->addr))
			continue;
		add_pool_entry(&pool, &m->pentry, &m->addr, &request);
	}

	/*
	 * There is heuristics to know when a packet is the last of the
	 * packet list sended by the master server.  However, UDP may
	 * reorder packets, and the last packet of the packet list may
	 * actually comes first.  In that case, we still want to wit for
	 * the remnaing packets.
	 *
	 * Since we don't have any reliable way to know when no more
	 * packets remains, we never manually remove a master from the
	 * pool.
	 */
	while ((entry = poll_pool(&pool, &data)))
		if (data)
			handle_data(data, list, get_master(entry));

	close_sockets(&sockets);
}

static int update_masters_info(void)
{
	int ret = 1;
	struct master_info *master;

	const char query[] =
		"UPDATE masters"
		" SET lastseen = ?"
		" WHERE node = ? AND service = ?";

	exec("BEGIN");

	for (master = masters; master->info.node[0]; master++) {
		struct master *m = &master->info;

		if (!master->is_online)
			continue;
		else
			m->lastseen = time(NULL);

		ret &= exec(query, "tss", m->lastseen, m->node, m->service);
	}

	exec("COMMIT");

	return ret;
}

static void update_server(struct server_addr *addr, struct master *master)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct server s;

	const char *query =
		"SELECT" ALL_SERVER_COLUMNS
		" FROM servers"
		" WHERE ip = ? AND port = ?";

	foreach_server(query, &s, "ss", addr->ip, addr->port);

	if (!res)
		return;

	if (!nrow) {
		create_server(addr->ip, addr->port, master->node, master->service);
		return;
	}

	snprintf(s.master_node, sizeof(s.master_node), "%s", master->node);
	snprintf(s.master_service, sizeof(s.master_service), "%s", master->service);
	write_server(&s);
}

int main(int argc, char **argv)
{
	struct list list;
	unsigned i;

	load_config(1);
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!init_masters_list())
		return EXIT_FAILURE;

	fill_list(&list);
	if (!update_masters_info())
		return EXIT_FAILURE;

	exec("BEGIN");

	exec("UPDATE servers SET master_node = '', master_service = ''");

	for (i = 0; i < list.length; i++) {
		struct server_addr *addr = &list.entries[i].addr;
		struct master *master = &list.entries[i].master->info;

		update_server(addr, master);
	}

	exec("COMMIT");

	verbose("Masters referenced %u servers\n", list.length);

	return EXIT_SUCCESS;
}
