#ifndef NETWORK_H
#define NETWORK_H

/*
 * For this project, only "connless" packets are used.  Those packets have
 * all the same header, so send_data() and recv_data() has been designed to
 * only send and receive connless packets.
 *
 * Teeworlds works with UDP, over ipv4 and ipv6 so two sockets are needed.
 * That's the purpose of struct socket.  Since we must have some sort of
 * timeout when waiting for data, recv_data() actually use poll().
 *
 * A struct data represent data without packet header.  Note that this data
 * also have their own header, different than the packet header.  We provide a
 * helper function to check the header and skip it (skip_header()).
 *
 * get_sockaddr() is a wrapper around getaddrinfo() that return only the first
 * adress found, handling every errors.
 */

#include <sys/socket.h>
#include <poll.h>

/* From teeworlds source code */
#define PACKET_SIZE 1400
#define PACKET_HEADER_SIZE 6

struct data {
	int size;
	unsigned char buffer[PACKET_SIZE - PACKET_HEADER_SIZE];
};

#define IP_LENGTH INET6_ADDRSTRLEN
#define PORT_LENGTH 10

enum ip_version {
	IPV4, IPV6
};

struct server_addr {
	enum ip_version version;
	char ip[IP_LENGTH + 1];
	char port[PORT_LENGTH + 1];
};

struct sockets {
	struct pollfd ipv4, ipv6;
};

int init_sockets(struct sockets *sockets);
void close_sockets(struct sockets *sockets);

int get_sockaddr(char *node, char *service, struct sockaddr_storage *addr);
int skip_header(struct data *data, const unsigned char *header, size_t size);

int send_data(
	struct sockets *sockets, const struct data *data,
	struct sockaddr_storage *addr);
int recv_data(
	struct sockets *sockets, struct data *data,
	struct sockaddr_storage *addr);

#endif /* NETWORK_H */
