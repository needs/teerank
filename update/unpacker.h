#ifndef UNPACKER_H
#define UNPACKER_H

#include "server.h"
#include "packet.h"

enum unpack_status {
	UNPACK_ERROR, UNPACK_SUCCESS, UNPACK_INCOMPLETE
};

enum unpack_status unpack_server_info(struct packet *packet, struct server *sv);
enum unpack_status unpack_server_addr(struct packet *packet, char **ip, char **port, int *reset_context);

#endif /* UNPACKER_H */
