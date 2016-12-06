#ifndef UNPACKER_H
#define UNPACKER_H

#include "server.h"
#include "packet.h"

int unpack_server_info(struct packet *packet, struct server *sv);
int unpack_server_addr(struct packet *packet, char **ip, char **port, int *reset_context);

#endif /* UNPACKER_H */
