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
			foreach_break;

	if (res)
		return 1;

	fprintf(
		stderr, "%s: init_masters_list(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
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
	struct data data;
	struct master_info *m;

	assert(list != NULL);

	list->length = 0;
	list->entries = NULL;

	if (!init_sockets(&sockets))
		exit(EXIT_FAILURE);

	data.size = sizeof(MSG_GETLIST);
	memcpy(data.buffer, MSG_GETLIST, data.size);
	init_pool(&pool, &sockets, &data);

	for (m = masters; m->info.node[0]; m++) {
		if (!get_sockaddr(m->info.node, m->info.service, &m->addr))
			continue;
		add_pool_entry(&pool, &m->pentry, &m->addr);
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
		handle_data(&data, list, get_master(entry));

	close_sockets(&sockets);
}

static int update_masters_info(void)
{
	struct master_info *master;
	sqlite3_stmt *res;
	const char query[] =
		"UPDATE masters"
		" SET lastseen = ?"
		" WHERE node = ? AND service = ?";

	if (sqlite3_exec(db, "BEGIN", 0, 0, 0) != SQLITE_OK)
		goto fail;

	if (sqlite3_prepare_v2(db, query, sizeof(query), &res, NULL) != SQLITE_OK)
		goto fail;

	for (master = masters; master->info.node[0]; master++) {
		if (!master->is_online)
			continue;
		else
			master->info.lastseen = time(NULL);

		if (sqlite3_bind_int64(res, 1, master->info.lastseen) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_text(res, 2, master->info.node, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;
		if (sqlite3_bind_text(res, 3, master->info.service, -1, SQLITE_STATIC) != SQLITE_OK)
			goto fail;

		if (sqlite3_step(res) != SQLITE_DONE)
			goto fail;

		if (sqlite3_reset(res) != SQLITE_OK)
			goto fail;
		if (sqlite3_clear_bindings(res) != SQLITE_OK)
			goto fail;
	}

	sqlite3_finalize(res);

	if (sqlite3_exec(db, "COMMIT", 0, 0, 0) != SQLITE_OK)
		goto fail;

	return 1;

fail:
	fprintf(
		stderr, "%s: update_masters_info(): %s\n",
		config.dbpath, sqlite3_errmsg(db));
	sqlite3_finalize(res);
	sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
	return 0;
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

	if (sqlite3_exec(db, "BEGIN", 0, 0, 0) != SQLITE_OK)
		return EXIT_FAILURE;

	for (i = 0; i < list.length; i++) {
		struct server_addr *addr = &list.entries[i].addr;
		struct master_info *master = list.entries[i].master;

		create_server(addr->ip, addr->port, master->info.node, master->info.service);
	}

	if (sqlite3_exec(db, "COMMIT", 0, 0, 0) != SQLITE_OK)
		return EXIT_FAILURE;

	verbose("Masters referenced %u servers\n", list.length);

	return EXIT_SUCCESS;
}
