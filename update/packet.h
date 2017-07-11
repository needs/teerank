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

#include "server.h"

/* From teeworlds source code */
#define PACKET_SIZE 1400

struct packet {
	/* Packet data */
	int size;
	uint8_t buffer[PACKET_SIZE];

	/* Unpacking state */
	size_t pos;
	bool error;
};

bool init_network(void);

void create_packet(struct packet *packet, uint8_t *data, int size);
bool get_sockaddr(char *node, char *service, struct sockaddr_storage *addr);
bool send_packet(const struct packet *packet, struct sockaddr_storage *addr);
bool recv_packet(struct packet *packet, struct sockaddr_storage *addr);

enum unpack_status {
	UNPACK_ERROR, UNPACK_SUCCESS, UNPACK_INCOMPLETE
};

enum unpack_status unpack_server_info(
	struct packet *packet, struct server *sv);
enum unpack_status unpack_server_addr(
	struct packet *packet, char **ip, char **port);

#endif /* PACKET_H */
