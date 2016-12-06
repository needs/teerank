#ifndef UNPACKER_H
#define UNPACKER_H

#include "server.h"
#include "network.h"

int unpack_server_info(struct data *data, struct server *sv);
int unpack_server_addr(struct data *data, char **ip, char **port, int *reset_context);

#endif /* UNPACKER_H */
