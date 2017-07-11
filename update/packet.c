#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <netdb.h>
#include <poll.h>

#include "packet.h"

/*
 * Teeworlds works with UDP, over ipv4 and ipv6 so two sockets are
 * needed.  That's the purpose of struct sockets.  Since we must have
 * some sort of timeout when waiting for data, recv_packet() actually
 * use poll().
 */
static struct sockets {
	struct pollfd ipv4, ipv6;
} sockets;

static void close_sockets(void)
{
	close(sockets.ipv4.fd);
	close(sockets.ipv6.fd);
}

bool init_network(void)
{
	sockets.ipv4.fd = socket(AF_INET , SOCK_DGRAM, IPPROTO_UDP);
	if (sockets.ipv4.fd == -1) {
		perror("socket(ipv4)");
		return false;
	}

	sockets.ipv6.fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sockets.ipv6.fd == -1) {
		perror("socket(ipv6)");
		close(sockets.ipv4.fd);
		return false;
	}

	atexit(close_sockets);
	return true;
}

bool send_packet(const struct packet *packet, struct sockaddr_storage *addr)
{
	unsigned char buf[CONNLESS_PACKET_SIZE];
	size_t bufsize;
	ssize_t ret;
	int fd;

	assert(sockets.ipv4.fd >= 0);
	assert(sockets.ipv6.fd >= 0);

	assert(packet != NULL);
	assert(packet->size > 0);
	assert(packet->size <= sizeof(packet->buffer));

	assert(addr != NULL);

	fd = addr->ss_family == AF_INET ? sockets.ipv4.fd : sockets.ipv6.fd;

	/*
	 * We only use connless packets.  We use the extended packet
	 * header as defined by ddrace so that we can support servers
	 * with more than 16 players.
	 */
	buf[0] = 'x';
	buf[1] = 'e';
	buf[2] = 0xff;
	buf[3] = 0xff;
	buf[4] = 0xff;
	buf[5] = 0xff;

	/* Just copy packet data as it is to the remaining space */
	memcpy(buf + CONNLESS_PACKET_HEADER_SIZE, packet->buffer, packet->size);

	bufsize = packet->size + CONNLESS_PACKET_HEADER_SIZE;
	ret = sendto(fd, buf, bufsize, 0, (struct sockaddr*)addr, sizeof(*addr));

	if (ret == -1) {
		perror("send()");
		return false;
	} else if (ret != bufsize) {
		fprintf(stderr,
		        "Packet has not been fully sent (%zu bytes over %zu)\n",
		        (size_t)ret, bufsize);
		return false;
	}

	return true;
}

bool recv_packet(struct packet *packet, struct sockaddr_storage *addr)
{
	unsigned char buf[CONNLESS_PACKET_SIZE];
	socklen_t addrlen = sizeof(addr);
	ssize_t ret;
	int fd;

	assert(sockets.ipv4.fd >= 0);
	assert(sockets.ipv6.fd >= 0);

	assert(packet != NULL);

	/* Poll ipv4 and ipv6 sockets */
	sockets.ipv4.events = POLLIN;
	sockets.ipv6.events = POLLIN;
	ret = poll((struct pollfd *)&sockets, 2, 1000);

	if (ret == -1) {
		if (errno != EINTR)
			perror("poll()");
		return false;
	} else if (ret == 0) {
		return false;
	}

	if (sockets.ipv4.revents & POLLIN)
		fd = sockets.ipv4.fd;
	else
		fd = sockets.ipv6.fd;

	ret = recvfrom(fd, buf, sizeof(buf), 0,
	               (struct sockaddr*)addr, addr ? &addrlen : NULL);
	if (ret == -1) {
		perror("recv()");
		return false;
	} else if (ret < CONNLESS_PACKET_HEADER_SIZE) {
		fprintf(stderr,
		        "Packet too small (%zu bytes required, %zu received)\n",
		        (size_t)CONNLESS_PACKET_HEADER_SIZE, (size_t)ret);
		return false;
	}

	/* Skip packet header (six bytes) */
	packet->size = ret - CONNLESS_PACKET_HEADER_SIZE;
	memcpy(packet->buffer, buf + CONNLESS_PACKET_HEADER_SIZE, packet->size);

	packet->pos = 0;
	packet->error = false;

	return true;
}

bool get_sockaddr(char *node, char *service, struct sockaddr_storage *addr)
{
	int ret;
	struct addrinfo hints, *res;

	assert(node != NULL);
	assert(service != NULL);
	assert(addr != NULL);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = 0;

	ret = getaddrinfo(node, service, &hints, &res);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo(%s, %s): %s\n",
		        node, service, gai_strerror(ret));
		return false;
	}

	/* Assume the first one works */
	*(struct sockaddr*)addr = *res->ai_addr;

	freeaddrinfo(res);
	return true;
}

static const uint8_t MSG_INFO[] = {
	255, 255, 255, 255, 'i', 'n', 'f', '3'
};
static const uint8_t MSG_LIST[] = {
	255, 255, 255, 255, 'l', 'i', 's', '2'
};
static const uint8_t MSG_INFO_64[] = {
	255, 255, 255, 255, 'd', 't', 's', 'f'
};
static const uint8_t MSG_INFO_EXTENDED[] = {
	255, 255, 255, 255, 'i', 'e', 'x', 't'
};
static const uint8_t MSG_INFO_EXTENDED_MORE[] = {
	255, 255, 255, 255, 'i', 'e', 'x', '+'
};

static bool can_unpack(struct packet *pck)
{
	assert(pck != NULL);
	return !pck->error && pck->pos < pck->size;
}

/*
 * Unpack the actual field and set the cursor to the next field.
 * Returns an empty string on error.
 */
static char *unpack(struct packet *pck)
{
	char *ret;

	assert(pck != NULL);

	if (!can_unpack(pck))
		goto fail;

	ret = (char*)&pck->buffer[pck->pos];
	for (; pck->buffer[pck->pos]; pck->pos++)
		if (pck->pos >= pck->size)
			goto fail;

	pck->pos++;
	return ret;

fail:
	pck->error = true;
	return "";
}

/*
 * It's a shame POSIX do not have a good enough function to copy strings
 * with bound check.  strncat() have its own caveat so we end up using
 * this suboptimal snprintf() trick.
 */
static void unpack_string(struct packet *pck, char *buf, size_t size)
{
	if (snprintf(buf, size, "%s", unpack(pck)) >= size)
		pck->error = true;
}

/*
 * Underflow, overflow and non numeric characters set unpacker's error
 * state.  Because non-malicious packets shouldn't contain such values.
 */
static long int unpack_int(struct packet *pck)
{
	long ret;
	char *str, *endptr;

	assert(pck != NULL);

	str = unpack(pck);
	errno = 0;
	ret = strtol(str, &endptr, 10);

	if (errno || !*str || *endptr)
		pck->error = true;

	return ret;
}

static bool skip_header(struct packet *packet, const uint8_t *header, size_t size)
{
	assert(packet != NULL);
	assert(header != NULL);
	assert(size > 0);

	if (packet->size < size)
		return false;
	if (memcmp(packet->buffer, header, size) != 0)
		return false;

	packet->pos += size;
	return true;
}

enum packet_type {
	VANILLA, LEGACY_64, EXTENDED, EXTENDED_MORE
};

static enum packet_type packet_type(struct packet *packet)
{
	if (skip_header(packet, MSG_INFO, sizeof(MSG_INFO)))
		return VANILLA;
	if (skip_header(packet, MSG_INFO_64, sizeof(MSG_INFO_64)))
		return LEGACY_64;
	if (skip_header(packet, MSG_INFO_EXTENDED, sizeof(MSG_INFO_EXTENDED)))
		return EXTENDED;
	if (skip_header(packet, MSG_INFO_EXTENDED_MORE, sizeof(MSG_INFO_EXTENDED_MORE)))
		return EXTENDED_MORE;

	return -1;
}

enum unpack_status unpack_server_info(struct packet *packet, struct server *sv)
{
	enum packet_type type;
	struct client *cl;

	assert(packet != NULL);
	assert(sv != NULL);

	if ((type = packet_type(packet)) == -1)
		return UNPACK_ERROR;

	/*
	 * Make things easier to read and helps identifying each fields,
	 * even the skipped ones.
	 */
	#define SKIP(var) unpack(packet)
	#define INT(var) var = unpack_int(packet)
	#define STRING(var) unpack_string(packet, var, sizeof(var))

	SKIP(token);

	if (type == EXTENDED_MORE)
		goto unpack_clients;

	SKIP(version);
	STRING(sv->name);
	STRING(sv->map);

	if (type == EXTENDED) {
		SKIP(map_crc);
		SKIP(map_size);
	}

	STRING(sv->gametype);
	SKIP(flags);
	SKIP(num_players);
	SKIP(max_players);
	INT(sv->num_clients);
	INT(sv->max_clients);

	/*
	 * We perform some sanity checks before going further to make
	 * sure the packet is well crafted.
	 */
	if (sv->num_clients > sv->max_clients)
		return UNPACK_ERROR;
	if (sv->num_clients > MAX_CLIENTS)
		return UNPACK_ERROR;
	if (sv->max_clients > MAX_CLIENTS)
		return UNPACK_ERROR;

unpack_clients:
	switch (type) {
	case VANILLA:
		break;
	case LEGACY_64:
		SKIP(offset);
		break;

	case EXTENDED_MORE:
		SKIP(pckno);
	case EXTENDED:
		SKIP(reserved);
		break;
	}

	/*
	 * We now unpack clients until there is no more clients to
	 * unpack.  Incomplete client info is considered failure, even
	 * though we could just discard it.
	 */
	while (can_unpack(packet) && (cl = new_client(sv))) {
		STRING(cl->name);
		STRING(cl->clan);
		SKIP(country);
		INT(cl->score);
		INT(cl->ingame);

		if (type == EXTENDED || type == EXTENDED_MORE)
			SKIP(reserved);
	}

	if (packet->error)
		return UNPACK_ERROR;
	else if (sv->received_clients < sv->num_clients)
		return UNPACK_INCOMPLETE;
	else
		return UNPACK_SUCCESS;

	#undef SKIP
	#undef INT
	#undef STRING
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
	static char ip[IP_STRSIZE], port[PORT_STRSIZE];
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

enum unpack_status unpack_server_addr(struct packet *packet, char **ip, char **port)
{
	assert(packet != NULL);
	assert(ip != NULL);
	assert(port != NULL);

	if (packet->pos == 0) {
		if (!skip_header(packet, MSG_LIST, sizeof(MSG_LIST)))
			return UNPACK_ERROR;
	}

	if (packet->size - packet->pos >= sizeof(struct server_addr_raw)) {
		struct server_addr_raw raw;

		memcpy(&raw, &packet->buffer[packet->pos], sizeof(raw));
		raw_addr_to_strings(&raw, ip, port);
		packet->pos += sizeof(raw);

		return UNPACK_INCOMPLETE;
	}

	return UNPACK_SUCCESS;
}
