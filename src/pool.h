#ifndef POOL_H
#define POOL_H

/*
 * A pool manage a group of servers and provide facilities to send messages
 * to them and receive answers in an efficient way from a networking point of
 * view.
 *
 * When receiving too much data over UDP, some of them will be lost.  One way
 * to mitigate the issue is to send request to a subset of x servers at a time.
 * That way, even if all answers comes at the same time, they won't fill
 * the bandwidth and just a few of them will be droped.
 *
 * Another way the pool deal with lost packets is by resending request after
 * some time.  A pool also have a timeout for each request send, so it can
 * efficiently detect when a UDP packet have probably been lost.
 */

#include "network.h"

struct pool_entry {
	struct sockaddr_storage *addr;
	unsigned failure_count;
	unsigned status;
	clock_t start_time;

	struct pool_entry *next_entry;
	struct pool_entry *next_pending;
};

struct pool {
	struct pool_entry *entries;
	struct pool_entry *pending, *iter;
	unsigned pending_count;

	struct sockets *sockets;
	const struct data *request;
};

/*
 * Initialize a pool.  Because no memory is allocated at initialization and
 * during pool lifetime, there is no free function.
 */
void init_pool(
	struct pool *pool, struct sockets *sockets, const struct data *request);

/*
 * Add a pool entry to the pool with the given adress.
 */
void add_pool_entry(
	struct pool *pool, struct pool_entry *entry,
	struct sockaddr_storage *addr);

/*
 * Poll the network and return the polled entry or NULL if any.
 */
struct pool_entry *poll_pool(struct pool *pool, struct data *answer);

#endif /* POOL_H */
