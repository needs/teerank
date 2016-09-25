#include <stdlib.h>
#include <stdio.h>
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

struct master {
	char *node, *service;
};

const struct master MASTERS[] = {
	{ "master1.teeworlds.com", "8300" },
	{ "master2.teeworlds.com", "8300" },
	{ "master3.teeworlds.com", "8300" },
	{ "master4.teeworlds.com", "8300" }
};
const unsigned MASTERS_LENGTH = sizeof(MASTERS) / sizeof(*MASTERS);

static const uint8_t MSG_GETLIST[] = {
	255, 255, 255, 255, 'r', 'e', 'q', '2'
};
static const uint8_t MSG_LIST[] = {
	255, 255, 255, 255, 'l', 'i', 's', '2'
};

struct server_list {
	unsigned length;
	struct server_addr *addrs;
};

static void add(struct server_list *list, struct server_addr *addr)
{
	const unsigned OFFSET = 1024;

	assert(list != NULL);
	assert(addr != NULL);

	if (list->length % OFFSET == 0) {
		list->addrs = realloc(
			list->addrs,
			(list->length + OFFSET) * sizeof(*list->addrs));
		if (!list->addrs) {
			perror("realloc(list)");
			exit(EXIT_FAILURE);
		}
	}

	list->addrs[list->length] = *addr;
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

static void handle_data(struct data *data, struct server_list *list)
{
	unsigned char *buf;
	int size;

	assert(data != NULL);
	assert(list != NULL);

	if (!skip_header(data, MSG_LIST, sizeof(MSG_LIST)))
		return;

	size = data->size;
	buf = data->buffer;

	while (size >= sizeof(struct server_addr_raw)) {
		struct server_addr_raw *raw = (struct server_addr_raw*)buf;
		struct server_addr addr;

		raw_addr_to_addr(raw, &addr);
		add(list, &addr);

		buf += sizeof(*raw);
		size -= sizeof(*raw);
	}
}

static void fill_server_list(struct server_list *list)
{
	struct sockets sockets;
	struct data data;
	int i;

	assert(list != NULL);

	list->length = 0;
	list->addrs = NULL;

	if (!init_sockets(&sockets))
		exit(EXIT_FAILURE);

	for (i = 0; i < MASTERS_LENGTH; i++) {
		struct sockaddr_storage addr;

		if (!get_sockaddr(MASTERS[i].node, MASTERS[i].service, &addr))
			continue;

		data.size = sizeof(MSG_GETLIST);
		memcpy(data.buffer, MSG_GETLIST, data.size);
		send_data(&sockets, &data, &addr);
	}

	while (recv_data(&sockets, &data, NULL) == 1)
		handle_data(&data, list);

	close_sockets(&sockets);
}

static char *addr_to_filename(struct server_addr *addr)
{
	static char ret[2 + 1 + IP_LENGTH + 1 + PORT_LENGTH + 1];
	unsigned i;

	assert(addr != NULL);

	/*
	 * Produce a unique identifier based on IP and port.
	 *
	 * 	<type> <IP> <port>
	 *
	 * Where every occurence of '.' and ':' has been replaced by '_'.
	 */

	sprintf(ret, "%s %s %s",
	        addr->version == IPV4 ? "v4" : "v6",
	        addr->ip, addr->port);

	for (i = 0; i < strlen(ret); i++)
		if (ret[i] == '.' || ret[i] == ':')
			ret[i] = '_';

	return ret;
}

int main(int argc, char **argv)
{
	struct server_list list;
	int fd;
	unsigned i;
	static char path[PATH_MAX];
	unsigned count_new = 0;

	load_config(1);
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	sprintf(path, "%s/servers", config.root);
	fd = open(path, O_RDONLY | O_DIRECTORY);
	if (fd == -1) {
		perror(path);
		return EXIT_FAILURE;
	}

	fill_server_list(&list);
	for (i = 0; i < list.length; i++) {
		char *filename = addr_to_filename(&list.addrs[i]);

		if (create_server(filename)) {
			verbose("New server: %s\n", filename);
			count_new++;
		}
	}

	close(fd);

	verbose("Over %u servers referenced by masters, %u are new\n",
	        list.length, count_new);

	return EXIT_SUCCESS;
}
