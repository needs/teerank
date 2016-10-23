#include <stdlib.h>
#include <assert.h>
#include <sys/times.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include "pool.h"
#include "network.h"

#define MAX_PENDING 25
#define MAX_FAILURE 2
#define MAX_PING 999

enum poll_status {
	IDLE, PENDING, POLLED, FAILED
};

void init_pool(
	struct pool *pool, struct sockets *sockets, const struct data *request,
	short resend_on_failure)
{
	static const struct pool POOL_ZERO;

	assert(pool != NULL);
	assert(sockets != NULL);
	assert(request != NULL);

	*pool = POOL_ZERO;

	pool->resend_on_failure = resend_on_failure;
	pool->sockets = sockets;
	pool->request = request;
}

void add_pool_entry(
	struct pool *pool, struct pool_entry *entry,
	struct sockaddr_storage *addr)
{
	static const struct pool_entry POOL_ENTRY_ZERO;

	assert(pool != NULL);
	assert(entry != NULL);
	assert(addr != NULL);

	*entry = POOL_ENTRY_ZERO;

	entry->addr = addr;
	entry->status = IDLE;
	entry->failure_count = 0;

	entry->next = pool->entries;
	pool->entries = entry;
}

static int entry_send_data(struct pool *pool, struct pool_entry *pentry)
{
	if (!pool->resend_on_failure && pentry->data_sent)
		return 1;

	return send_data(pool->sockets, pool->request, pentry->addr);
}

static void add_pending_entry(struct pool *pool, struct pool_entry *entry)
{
	assert(pool != NULL);
	assert(entry != NULL);
	assert(pool->pending_count < MAX_PENDING);

	if (!entry_send_data(pool, entry))
		return;

	entry->data_sent = 1;
	entry->start_time = times(NULL);
	entry->status = PENDING;

	/* Insert before list head */
	entry->prev_pending = NULL;
	entry->next_pending = pool->pending;
	if (pool->pending)
		pool->pending->prev_pending = entry;
	pool->pending = entry;
	pool->pending_count++;
}

/*
 * Iterate over every pollable entries, starting
 * from the last iterated entry.
 */
static struct pool_entry *foreach_entries(struct pool *pool)
{
	struct pool_entry *iter;

	assert(pool != NULL);

	for (iter = pool->iter; iter; iter = iter->next) {
		if (iter->status == IDLE) {
			pool->iter = iter->next;
			return iter;
		}
	}

	/* Start over */
	for (iter = pool->entries; iter != pool->iter; iter = iter->next) {
		if (iter->status == IDLE) {
			pool->iter = iter->next;
			return iter;
		}
	}

	return NULL;
}

static void fill_pending_list(struct pool *pool)
{
	struct pool_entry *entry;

	assert(pool != NULL);

	while (pool->pending_count < MAX_PENDING
	       && (entry = foreach_entries(pool))) {
		add_pending_entry(pool, entry);
	}
}

/*
 * Remove the given pool entry from the pending list, but keep the given
 * entry pointers set, so that removing while looping still works.
 */
static void remove_pending_entry(
	struct pool *pool, struct pool_entry *entry)
{
	assert(pool != NULL);
	assert(entry != NULL);
	assert(entry->prev_pending != NULL || entry == pool->pending);

	if (entry->next_pending)
		entry->next_pending->prev_pending = entry->prev_pending;
	if (entry->prev_pending)
		entry->prev_pending->next_pending = entry->next_pending;

	if (entry == pool->pending) {
		pool->pending = entry->next_pending;
		if (pool->pending)
			pool->pending->prev_pending = NULL;
	}

	pool->pending_count--;
}

void remove_pool_entry(struct pool *pool, struct pool_entry *pentry)
{
	if (pentry->status == PENDING)
		remove_pending_entry(pool, pentry);

	/* TODO: Remove it from the pool as well */
	pentry->status = POLLED;
}

static void clean_old_pending_entries(struct pool *pool)
{
	static long ticks_per_second = -1;
	struct pool_entry *entry;
	clock_t now;

	assert(pool != NULL);

	/* Set ticks_per_seconds once for all */
	if (ticks_per_second == -1)
		ticks_per_second = sysconf(_SC_CLK_TCK);
	assert(ticks_per_second != -1);

	now = times(NULL);

	for (entry = pool->pending; entry; entry = entry->next_pending) {
		/* In seconds */
		float elapsed = (now - entry->start_time) / ticks_per_second;

		if (elapsed >= MAX_PING / 1000.0f) {
			remove_pending_entry(pool, entry);

			if (entry->failure_count == MAX_FAILURE)
				entry->status = FAILED;
			else
				entry->status = IDLE;

			entry->failure_count++;
		}
	}
}

static int is_same_addr(
	struct sockaddr_storage *_a, struct sockaddr_storage *_b)
{
	assert(_a != NULL);
	assert(_b != NULL);

	assert(_a->ss_family == AF_INET || _a->ss_family == AF_INET6);
	assert(_b->ss_family == AF_INET || _b->ss_family == AF_INET6);

	if (_a->ss_family != _b->ss_family)
		return 0;

	if (_a->ss_family == AF_INET) {
		struct sockaddr_in *a = (struct sockaddr_in*)_a;
		struct sockaddr_in *b = (struct sockaddr_in*)_b;

		if (memcmp(&a->sin_addr, &b->sin_addr, sizeof(a->sin_addr)))
			return 0;
		if (memcmp(&a->sin_port, &b->sin_port, sizeof(a->sin_port)))
			return 0;
	} else {
		struct sockaddr_in6 *a = (struct sockaddr_in6*)_a;
		struct sockaddr_in6 *b = (struct sockaddr_in6*)_b;

		if (memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(a->sin6_addr)))
			return 0;
		if (memcmp(&a->sin6_port, &b->sin6_port, sizeof(a->sin6_port)))
			return 0;
	}

	return 1;
}

static struct pool_entry *get_pending_entry(
	struct pool *pool, struct sockaddr_storage *addr)
{
	struct pool_entry *entry;

	assert(pool != NULL);
	assert(addr != NULL);
	assert(pool->pending_count > 0);

	for (entry = pool->pending; entry; entry = entry->next_pending)
		if (is_same_addr(addr, entry->addr))
			return entry;

	return NULL;
}

struct pool_entry *poll_pool(struct pool *pool, struct data *answer)
{
	struct sockaddr_storage addr;
	struct pool_entry *entry = NULL;

	assert(pool != NULL);
	assert(answer != NULL);

	goto start;
	while (pool->pending_count > 0) {
		if (recv_data(pool->sockets, answer, &addr))
			entry = get_pending_entry(pool, &addr);
		else
			entry = NULL;

	start:
		clean_old_pending_entries(pool);
		fill_pending_list(pool);

		if (entry)
			return entry;
	}

	return NULL;
}

struct pool_entry *foreach_failed_poll(struct pool *pool)
{
	struct pool_entry *iter;

	assert(pool != NULL);

	if (!pool->iter_failed)
		pool->iter_failed = pool->entries;

	for (iter = pool->iter_failed; iter; iter = iter->next) {
		if (iter->status == FAILED) {
			pool->iter_failed = iter->next;
			return iter;
		}
	}

	pool->iter_failed = NULL;
	return NULL;
}
