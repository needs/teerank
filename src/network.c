#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#include "network.h"

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

int send_data(
	struct sockets *sockets, const struct data *data,
	struct sockaddr_storage *addr)
{
	unsigned char packet[PACKET_SIZE];
	int fd, ret;

	assert(sockets != NULL);
	assert(sockets->ipv4.fd >= 0);
	assert(sockets->ipv6.fd >= 0);

	assert(data != NULL);
	assert(data->size > 0);
	assert(data->size <= sizeof(data->buffer));

	assert(addr != NULL);

	fd = addr->ss_family == AF_INET ? sockets->ipv4.fd : sockets->ipv6.fd;

	/*
	 * We only use connless packets for our needs.  Connless packets
	 * should have the first six bytes of packet's header set to 0xff.
	 */
	packet[0] = 0xff;
	packet[1] = 0xff;
	packet[2] = 0xff;
	packet[3] = 0xff;
	packet[4] = 0xff;
	packet[5] = 0xff;

	/* Just copy data as it is to the remaining space */
	memcpy(packet + PACKET_HEADER_SIZE, data->buffer, data->size);

	ret = sendto(fd, packet, data->size + PACKET_HEADER_SIZE, 0,
	             (struct sockaddr*)addr, sizeof(*addr));
	if (ret == -1) {
		perror("send()");
		return 0;
	} else if (ret != data->size + PACKET_HEADER_SIZE) {
		fprintf(stderr,
		        "Data has not been fully sent (%d bytes over %d)\n",
		        ret, data->size + PACKET_HEADER_SIZE);
		return 0;
	}

	return 1;
}

int recv_data(
	struct sockets *sockets, struct data *data,
	struct sockaddr_storage *addr)
{
	unsigned char packet[PACKET_SIZE];
	int ret, fd;
	socklen_t addrlen = sizeof(addr);

	assert(sockets != NULL);
	assert(sockets->ipv4.fd >= 0);
	assert(sockets->ipv6.fd >= 0);

	assert(data != NULL);

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

	ret = recvfrom(fd, packet, sizeof(packet), 0,
	               (struct sockaddr*)addr, addr ? &addrlen : NULL);
	if (ret == -1) {
		perror("recv()");
		return 0;
	} else if (ret < PACKET_HEADER_SIZE) {
		fprintf(stderr,
		        "Packet too small (%d bytes required, %d received)\n",
		        PACKET_HEADER_SIZE, ret);
		return 0;
	}

	/* Skip packet header (six bytes) */
	data->size = ret - PACKET_HEADER_SIZE;
	memcpy(data->buffer, packet + PACKET_HEADER_SIZE, data->size);

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

int skip_header(struct data *data, const unsigned char *header, size_t size)
{
	assert(data != NULL);
	assert(header != NULL);
	assert(size > 0);

	if (data->size < size)
		return 0;
	if (memcmp(data->buffer, header, size) != 0)
		return 0;

	data->size -= size;
	memmove(data->buffer, &data->buffer[size], data->size);
	return 1;
}
