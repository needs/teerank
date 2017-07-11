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

bool skip_header(struct packet *packet, const uint8_t *header, size_t size)
{
	assert(packet != NULL);
	assert(header != NULL);
	assert(size > 0);

	if (packet->size < size)
		return false;
	if (memcmp(packet->buffer, header, size) != 0)
		return false;

	packet->size -= size;
	memmove(packet->buffer, &packet->buffer[size], packet->size);
	return true;
}
