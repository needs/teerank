#include <stdlib.h>
#include <assert.h>
#include <sys/times.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include "pool.h"
#include "network.h"

#define MAX_PENDING 25
#define MAX_RETRIES 2
#define MAX_PING 999

void init_pool(
	struct pool *pool, struct sockets *sockets, const struct data *request)
{
	static const struct pool POOL_ZERO;

	assert(pool != NULL);
	assert(sockets != NULL);
	assert(request != NULL);

	*pool = POOL_ZERO;

	pool->sockets = sockets;
	pool->request = request;
}

/*
 * Helpers to insert and remove a given entry from the given double
 * linked list.  Since this operation can be tricky we better have to
 * write it once and reuse it.
 */
static void insert_entry(
	struct pool_entry *entry,
	struct pool_entry **head,
	struct pool_entry **tail)
{
	assert(entry != NULL);
	assert(head != NULL);

	if (*head)
		(*head)->prev = entry;
	else if (tail)
		(*tail) = entry;

	entry->next = (*head);
	entry->prev = NULL;
	(*head) = entry;
}

static void remove_entry(
	struct pool_entry *entry,
	struct pool_entry **head,
	struct pool_entry **tail)
{
	assert(entry != NULL);
	assert(head != NULL);

	if (entry->prev)
		entry->prev->next = entry->next;
	if (entry->next)
		entry->next->prev = entry->prev;
	if (*head == entry)
		(*head) = entry->next;
	if (tail && *tail == entry)
		(*tail) = entry->prev;
}

/*
 * Should work even if the current iterated entry is moved to another
 * list (hence making ->next meaningless for the current loop).
 */
#define list_foreach(list, it, _next) for ( \
	it = list; \
	_next = (it ? it->next : NULL), it; \
	it = _next \
)

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

	insert_entry(entry, &pool->idle, &pool->idletail);
}

static void entry_expired(struct pool *pool, struct pool_entry *entry)
{
	if (entry->polled)
		return;

	if (entry->retries == MAX_RETRIES) {
		insert_entry(entry, &pool->failed, NULL);
	} else {
		insert_entry(entry, &pool->idle, &pool->idletail);
		entry->retries++;
	}
}

static void add_pending_entry(struct pool *pool, struct pool_entry *entry)
{
	assert(pool != NULL);
	assert(entry != NULL);
	assert(pool->npending < MAX_PENDING);

	remove_entry(entry, &pool->idle, &pool->idletail);

	if (!send_data(pool->sockets, pool->request, entry->addr)) {
		entry_expired(pool, entry);
		return;
	}

	entry->start_time = times(NULL);

	insert_entry(entry, &pool->pending, NULL);
	pool->npending++;
}

void remove_pool_entry(struct pool *pool, struct pool_entry *entry)
{
	remove_entry(entry, &pool->pending, NULL);
	pool->npending--;
}

static void fill_pending_list(struct pool *pool)
{
	assert(pool != NULL);

	while (pool->idletail && pool->npending < MAX_PENDING)
		add_pending_entry(pool, pool->idletail);
}

static void clean_expired_pending_entries(struct pool *pool)
{
	static long ticks_per_second = -1;
	struct pool_entry *entry, *next;
	clock_t now;

	assert(pool != NULL);

	/* Set ticks_per_seconds once for all */
	if (ticks_per_second == -1)
		ticks_per_second = sysconf(_SC_CLK_TCK);
	assert(ticks_per_second != -1);

	now = times(NULL);

	list_foreach(pool->pending, entry, next) {
		/* In seconds */
		float elapsed = (now - entry->start_time) / ticks_per_second;

		if (elapsed >= MAX_PING / 1000.0f) {
			remove_entry(entry, &pool->pending, NULL);
			pool->npending--;
			entry_expired(pool, entry);
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
	struct pool_entry *entry, *next;

	assert(pool != NULL);
	assert(addr != NULL);

	list_foreach(pool->pending, entry, next)
		if (is_same_addr(addr, entry->addr))
			break;

	if (entry) {
		entry->polled = 1;
		entry->start_time = times(NULL);
	}

	return entry;
}

static struct pool_entry *next_failed_entry(struct pool *pool)
{
	struct pool_entry *entry;

	clean_expired_pending_entries(pool);
	fill_pending_list(pool);

	if (!pool->failed)
		return NULL;

	entry = pool->failed;
	remove_entry(entry, &pool->failed, NULL);

	return entry;
}

struct pool_entry *poll_pool(struct pool *pool, struct data **answer)
{
	struct sockaddr_storage addr;
	struct pool_entry *entry = NULL;

	assert(pool != NULL);
	assert(answer != NULL);

again:
	fill_pending_list(pool);
	if ((entry = next_failed_entry(pool))) {
		*answer = NULL;
		return entry;
	}

	if (pool->pending) {
		if (recv_data(pool->sockets, &pool->databuf, &addr))
			entry = get_pending_entry(pool, &addr);

		if (entry) {
			*answer = &pool->databuf;
			return entry;
		} else {
			clean_expired_pending_entries(pool);
			goto again;
		}
	}

	return NULL;
}
