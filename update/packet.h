#ifndef PACKET_H
#define PACKET_H

/*
 * For this project, only "connless" packets are used.  Those packets
 * have all the same header, so send_packet() and recv_packet() has been
 * designed to only send and receive connless packets.
 *
 * A struct packet represent data without connless packet header.  Since
 * we only work with connless packet, we ignore their header right away.
 *
 * get_sockaddr() is a wrapper around getaddrinfo() that return only the
 * first adress found, handling every errors.
 */

#include <stdbool.h>
#include <sys/socket.h>
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

bool init_network(void);

bool get_sockaddr(char *node, char *service, struct sockaddr_storage *addr);
bool skip_header(struct packet *packet, const uint8_t *header, size_t size);

bool send_packet(const struct packet *packet, struct sockaddr_storage *addr);
bool recv_packet(struct packet *packet, struct sockaddr_storage *addr);

#endif /* PACKET_H */
