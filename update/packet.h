#ifndef PACKET_H
#define PACKET_H

/*
 * For this project, only "connless" packets are used.  Those packets
 * have all the same header, so send_packet() and recv_packet() has been
 * designed to only send and receive connless packets.
 *
 * Teeworlds works with UDP, over ipv4 and ipv6 so two sockets are
 * needed.  That's the purpose of struct socket.  Since we must have
 * some sort of timeout when waiting for data, recv_packet() actually
 * use poll().
 *
 * A struct packet represent data without connless packet header.  Since
 * we only work with connless packet, we ignore their header right away.
 *
 * get_sockaddr() is a wrapper around getaddrinfo() that return only the
 * first adress found, handling every errors.
 */

#include <sys/socket.h>
#include <poll.h>
#include <stdint.h>
#include <netinet/in.h>

/* From teeworlds source code */
#define CONNLESS_PACKET_SIZE 1400
#define CONNLESS_PACKET_HEADER_SIZE 6

#define PACKET_SIZE (CONNLESS_PACKET_SIZE - CONNLESS_PACKET_HEADER_SIZE)

struct packet {
	int size;
	uint8_t buffer[PACKET_SIZE];
};

struct sockets {
	struct pollfd ipv4, ipv6;
};

int init_sockets(struct sockets *sockets);
void close_sockets(struct sockets *sockets);

int get_sockaddr(char *node, char *service, struct sockaddr_storage *addr);
int skip_header(struct packet *packet, const uint8_t *header, size_t size);

int send_packet(
	struct sockets *sockets, const struct packet *packet,
	struct sockaddr_storage *addr);
int recv_packet(
	struct sockets *sockets, struct packet *packet,
	struct sockaddr_storage *addr);

#endif /* PACKET_H */
