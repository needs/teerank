#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#include "packet.h"

int init_sockets(struct sockets *sockets)
{
	assert(sockets != NULL);

	sockets->ipv4.fd = socket(AF_INET , SOCK_DGRAM, IPPROTO_UDP);
	if (sockets->ipv4.fd == -1) {
		perror("socket(ipv4)");
		return 0;
	}

	sockets->ipv6.fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sockets->ipv6.fd == -1) {
		perror("socket(ipv6)");
		close(sockets->ipv4.fd);
		return 0;
	}

	return 1;
}

void close_sockets(struct sockets *sockets)
{
	assert(sockets != NULL);
	close(sockets->ipv4.fd);
	close(sockets->ipv6.fd);
}

int send_packet(
	struct sockets *sockets, const struct packet *packet,
	struct sockaddr_storage *addr)
{
	unsigned char buf[CONNLESS_PACKET_SIZE];
	size_t bufsize;
	ssize_t ret;
	int fd;

	assert(sockets != NULL);
	assert(sockets->ipv4.fd >= 0);
	assert(sockets->ipv6.fd >= 0);

	assert(packet != NULL);
	assert(packet->size > 0);
	assert(packet->size <= sizeof(packet->buffer));

	assert(addr != NULL);

	fd = addr->ss_family == AF_INET ? sockets->ipv4.fd : sockets->ipv6.fd;

	/*
	 * We only use connless packets for our needs.  Connless packets
	 * should have the first six bytes of packet's header set to 0xff.
	 */
	buf[0] = 0xff;
	buf[1] = 0xff;
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
		return 0;
	} else if (ret != bufsize) {
		fprintf(stderr,
		        "Packet has not been fully sent (%zu bytes over %zu)\n",
		        (size_t)ret, bufsize);
		return 0;
	}

	return 1;
}

int recv_packet(
	struct sockets *sockets, struct packet *packet,
	struct sockaddr_storage *addr)
{
	unsigned char buf[CONNLESS_PACKET_SIZE];
	socklen_t addrlen = sizeof(addr);
	ssize_t ret;
	int fd;

	assert(sockets != NULL);
	assert(sockets->ipv4.fd >= 0);
	assert(sockets->ipv6.fd >= 0);

	assert(packet != NULL);

	/* Poll ipv4 and ipv6 sockets */
	sockets->ipv4.events = POLLIN;
	sockets->ipv6.events = POLLIN;
	ret = poll((struct pollfd*)sockets, 2, 1000);

	if (ret == -1) {
		perror("poll()");
		return 0;
	} else if (ret == 0) {
		return 0;
	}

	if (sockets->ipv4.revents & POLLIN)
		fd = sockets->ipv4.fd;
	else
		fd = sockets->ipv6.fd;

	ret = recvfrom(fd, buf, sizeof(buf), 0,
	               (struct sockaddr*)addr, addr ? &addrlen : NULL);
	if (ret == -1) {
		perror("recv()");
		return 0;
	} else if (ret < CONNLESS_PACKET_HEADER_SIZE) {
		fprintf(stderr,
		        "Packet too small (%zu bytes required, %zu received)\n",
		        (size_t)CONNLESS_PACKET_HEADER_SIZE, (size_t)ret);
		return 0;
	}

	/* Skip packet header (six bytes) */
	packet->size = ret - CONNLESS_PACKET_HEADER_SIZE;
	memcpy(packet->buffer, buf + CONNLESS_PACKET_HEADER_SIZE, packet->size);

	return 1;
}

int get_sockaddr(char *node, char *service, struct sockaddr_storage *addr)
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
		return 0;
	}

	/* Assume the first one works */
	*(struct sockaddr*)addr = *res->ai_addr;

	freeaddrinfo(res);
	return 1;
}

int skip_header(struct packet *packet, const uint8_t *header, size_t size)
{
	assert(packet != NULL);
	assert(header != NULL);
	assert(size > 0);

	if (packet->size < size)
		return 0;
	if (memcmp(packet->buffer, header, size) != 0)
		return 0;

	packet->size -= size;
	memmove(packet->buffer, &packet->buffer[size], packet->size);
	return 1;
}
