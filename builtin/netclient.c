#include "netclient.h"

#define MAX_NETCLIENTS 4096

static struct netclient netclients[MAX_NETCLIENTS];
static struct netclient *nextfree;

/* Build freelist */
static void init(void)
{
	static int initialized;
	unsigned i;

	if (initialized)
		return;

	for (i = 0; i < MAX_NETCLIENTS-1; i++)
		netclients[i].nextfree = &netclients[i+1];

	nextfree = netclients;
	initialized = 1;
}

static struct netclient *new_netclient(void)
{
	static const struct netclient NETCLIENT_ZERO;
	struct netclient *netclient;

	init();

	if (!nextfree)
		return NULL;

	netclient = nextfree;
	nextfree = nextfree->nextfree;

	*netclient = NETCLIENT_ZERO;
	return netclient;
}

struct netclient *add_netclient(enum netclient_type type, void *info)
{
	struct netclient *netclient = new_netclient();
	char *node, *service;

	if (!netclient)
		return NULL;

	switch (type) {
	case NETCLIENT_TYPE_SERVER:
		netclient->info.server = *(struct server*)info;

		node = netclient->info.server.ip;
		service = netclient->info.server.port;
		break;

	case NETCLIENT_TYPE_MASTER:
		netclient->info.master = *(struct master*)info;

		node = netclient->info.master.node;
		service = netclient->info.master.service;
		break;
	default:
		return NULL;
	}

	if (!get_sockaddr(node, service, &netclient->addr)) {
		remove_netclient(netclient);
		return NULL;
	}

	netclient->type = type;
	return netclient;
}

void remove_netclient(struct netclient *netclient)
{
	netclient->nextfree = nextfree;
	nextfree = netclient;
}
