#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "pool.h"
#include "scheduler.h"
#include "server.h"
#include "master.h"

enum netclient_type {
	NETCLIENT_TYPE_SERVER,
	NETCLIENT_TYPE_MASTER
};

struct netclient {
	struct sockaddr_storage addr;
	struct pool_entry pentry;
	struct job update;

	enum netclient_type type;
	union {
		struct server server;
		struct master master;
	} info;

	struct netclient *nextfree;
};

struct netclient *add_netclient(enum netclient_type type, void *info);
void remove_netclient(struct netclient *netclient);

#endif /* NETCLIENT_H */
