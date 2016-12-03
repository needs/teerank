#ifndef POOL_H
#define POOL_H

/**
 * @file pool.h
 *
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

/**
 * @struct pool_entry
 *
 * A data strcuture containing this following structure can be added to the
 * pool.  This structure hold all the necessary data the pool need to know
 * to do its job.
 */
struct pool_entry {
	struct sockaddr_storage *addr;
	const struct data *request;

	unsigned retries;
	short polled;
	clock_t start_time;

	struct pool_entry *next, *prev;
};

/**
 * @struct pool
 *
 * Hold pool states.
 */
struct pool {
	struct pool_entry *idle, *idletail, *pending, *failed;
	unsigned npending;

	struct sockets *sockets;
	struct data databuf;
};

/**
 * Initialize a pool.  Because no memory is allocated at initialization and
 * during pool lifetime, there is no free function.
 *
 * @param pool Pool to initialize
 * @param sockets An initialized socket structure
 * @param request Data to send to pool entries
 */
void init_pool(
	struct pool *pool, struct sockets *sockets);

/**
 * Add a pool entry to the pool with the given adress.
 *
 * @param pool Pool to add the entry to
 * @param entry Entry to add to the pool
 * @param addr Network address for the given entry
 */
void add_pool_entry(
	struct pool *pool, struct pool_entry *entry,
	struct sockaddr_storage *addr, const struct data *request);

/**
 * Remove entry from the pool and the pending list
 *
 * @param pool Pool to remove entry from
 * @param pentry Entry to remove
 */
void remove_pool_entry(struct pool *pool, struct pool_entry *pentry);

/**
 * Poll the network and return the first entry we got an anwser from.
 *
 * If the retuned entry is not manually removed from the pool using
 * remove_pool_entry(), then it will be polled again until failure.
 *
 * @param pool Pool to poll
 * @param Received answer
 *
 * @return Polled entry, NULL if any
 */
struct pool_entry *poll_pool(struct pool *pool, struct data **data);

#endif /* POOL_H */
