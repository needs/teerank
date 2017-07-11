#include <stdlib.h>
#include <assert.h>
#include <sys/times.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include "pool.h"
#include "packet.h"

#define MAX_PENDING 25
#define MAX_RETRIES 2
#define MAX_PING 999

static struct pool_entry *idle, *idletail;
static struct pool_entry *pending;
static struct pool_entry *failed;
static unsigned nr_pending;

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
	struct pool_entry *entry,
	struct sockaddr_storage *addr, const struct packet *request)
{
	static const struct pool_entry POOL_ENTRY_ZERO;

	assert(entry != NULL);
	assert(addr != NULL);

	*entry = POOL_ENTRY_ZERO;
	entry->addr = addr;
	entry->request = request;

	insert_entry(entry, &idle, &idletail);
}

static void entry_expired(struct pool_entry *entry)
{
	if (entry->retries == MAX_RETRIES) {
		insert_entry(entry, &failed, NULL);
	} else {
		insert_entry(entry, &idle, &idletail);
		entry->retries++;
	}
}

static void add_pending_entry(struct pool_entry *entry)
{
	assert(entry != NULL);
	assert(nr_pending < MAX_PENDING);

	remove_entry(entry, &idle, &idletail);

	if (!send_packet(entry->request, entry->addr)) {
		entry_expired(entry);
		return;
	}

	entry->start_time = times(NULL);

	insert_entry(entry, &pending, NULL);
	nr_pending++;
}

void remove_pool_entry(struct pool_entry *entry)
{
	remove_entry(entry, &pending, NULL);
	nr_pending--;
}

static void fill_pending_list(void)
{
	while (idletail && nr_pending < MAX_PENDING)
		add_pending_entry(idletail);
}

static void clean_expired_pending_entries(void)
{
	static long ticks_per_second = -1;
	struct pool_entry *entry, *next;
	clock_t now;

	/* Set ticks_per_seconds once for all */
	if (ticks_per_second == -1)
		ticks_per_second = sysconf(_SC_CLK_TCK);
	assert(ticks_per_second != -1);

	now = times(NULL);

	list_foreach(pending, entry, next) {
		/* In seconds */
		float elapsed = (now - entry->start_time) / ticks_per_second;

		if (elapsed >= MAX_PING / 1000.0f) {
			remove_entry(entry, &pending, NULL);
			nr_pending--;
			entry_expired(entry);
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
	struct sockaddr_storage *addr)
{
	struct pool_entry *entry, *next;

	assert(addr != NULL);

	list_foreach(pending, entry, next)
		if (is_same_addr(addr, entry->addr))
			break;

	if (entry) {
		entry->polled = 1;
		entry->start_time = times(NULL);
	}

	return entry;
}

static struct pool_entry *next_failed_entry(void)
{
	struct pool_entry *entry;

	clean_expired_pending_entries();
	fill_pending_list();

	if (!failed)
		return NULL;

	entry = failed;
	remove_entry(entry, &failed, NULL);

	return entry;
}

struct pool_entry *poll_pool(struct packet **_packet)
{
	static struct packet packet;
	struct sockaddr_storage addr;
	struct pool_entry *entry = NULL;

	assert(_packet != NULL);

again:
	fill_pending_list();
	if ((entry = next_failed_entry())) {
		*_packet = NULL;
		return entry;
	}

	if (pending) {
		if (recv_packet(&packet, &addr))
			entry = get_pending_entry(&addr);

		if (entry) {
			*_packet = &packet;
			return entry;
		} else {
			clean_expired_pending_entries();
			goto again;
		}
	}

	return NULL;
}
