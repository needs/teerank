#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "unpacker.h"

static const uint8_t MSG_INFO[] = {
	255, 255, 255, 255, 'i', 'n', 'f', '3'
};
static const uint8_t MSG_LIST[] = {
	255, 255, 255, 255, 'l', 'i', 's', '2'
};

struct unpacker {
	struct packet *packet;
	size_t offset;
};

static void init_unpacker(struct unpacker *up, struct packet *packet)
{
	assert(up != NULL);
	assert(packet != NULL);

	up->packet = packet;
	up->offset = 0;
}

static int can_unpack(struct unpacker *up, unsigned length)
{
	unsigned offset;

	assert(up != NULL);
	assert(length > 0);

	for (offset = up->offset; offset < up->packet->size; offset++)
		if (up->packet->buffer[offset] == 0)
			if (--length == 0)
				return 1;

	return 0;
}

static char *unpack(struct unpacker *up)
{
	size_t old_offset;

	assert(up != NULL);

	old_offset = up->offset;
	while (up->offset < up->packet->size
	       && up->packet->buffer[up->offset] != 0)
		up->offset++;

	/* Skip the remaining 0 */
	up->offset++;

	/* can_unpack() should have been used */
	assert(up->offset <= up->packet->size);

	return (char*)&up->packet->buffer[old_offset];
}

static void skip_field(struct unpacker *up)
{
	unpack(up);
}

/*
 * It's a shame POSIX do not have a good enough function to copy strings
 * with bound check.  strncat() have its own caveat so we end up using
 * this suboptimal snprintf() trick.
 */
static void unpack_string(struct unpacker *up, char *buf, size_t size)
{
	snprintf(buf, size, "%s", unpack(up));
}

static long int unpack_int(struct unpacker *up)
{
	long ret;
	char *str, *endptr;

	assert(up != NULL);

	str = unpack(up);
	errno = 0;
	ret = strtol(str, &endptr, 10);

	if (errno == ERANGE && ret == LONG_MIN)
		fprintf(stderr, "unpack_int(%s): Underflow, value truncated\n", str);
	else if (errno == ERANGE && ret == LONG_MAX)
		fprintf(stderr, "unpack_int(%s): Overflow, value truncated\n", str);
	else if (endptr == str)
		fprintf(stderr, "unpack_int(%s): Cannot convert string\n", str);
	return ret;
}

int unpack_server_info(struct packet *packet, struct server *sv)
{
	struct unpacker up;
	unsigned i;

	/*
	 * Keep old data so that we an restore them in case of
	 * failure.
	 */
	struct server old;

	assert(packet != NULL);
	assert(sv != NULL);

	if (!skip_header(packet, MSG_INFO, sizeof(MSG_INFO)))
		return 0;

	init_unpacker(&up, packet);
	old = *sv;

	if (!can_unpack(&up, 10))
		goto fail;

	skip_field(&up); /* Token */
	skip_field(&up); /* Version */
	unpack_string(&up, sv->name,     sizeof(sv->name));     /* Name */
	unpack_string(&up, sv->map,      sizeof(sv->map));      /* Map */
	unpack_string(&up, sv->gametype, sizeof(sv->gametype)); /* Gametype */

	skip_field(&up); /* Flags */
	skip_field(&up); /* Player number */
	skip_field(&up); /* Player max number */
	sv->num_clients = unpack_int(&up); /* Client number */
	sv->max_clients = unpack_int(&up); /* Client max number */

	if (sv->num_clients > MAX_CLIENTS)
		goto fail;
	if (sv->max_clients > MAX_CLIENTS)
		goto fail;
	if (sv->num_clients > sv->max_clients)
		goto fail;

	/* Clients */
	for (i = 0; i < sv->num_clients; i++) {
		struct client *cl = &sv->clients[i];

		if (!can_unpack(&up, 5))
			goto fail;

		unpack_string(&up, cl->name, sizeof(cl->name)); /* Name */
		unpack_string(&up, cl->clan, sizeof(cl->clan)); /* Clan */

		skip_field(&up); /* Country */
		cl->score  = unpack_int(&up); /* Score */
		cl->ingame = unpack_int(&up); /* Ingame? */
	}

	return 1;
fail:
	*sv = old;
	return 0;
}

struct server_addr_raw {
	uint8_t ip[16];
	uint8_t port[2];
};

static int is_ipv4(unsigned char *ip)
{
	static const unsigned char IPV4_HEADER[] = {
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0xFF
	};

	assert(ip != NULL);

	return memcmp(ip, IPV4_HEADER, sizeof(IPV4_HEADER)) == 0;
}

static void raw_addr_to_strings(
	struct server_addr_raw *raw, char **_ip, char **_port)
{
	static char ip[IP_STRSIZE], port[PORT_STRSIZE];
	uint16_t portnum;
	int ret;

	assert(raw != NULL);
	assert(_ip != NULL);
	assert(_port != NULL);

	/* Convert raw adress to either ipv4 or ipv6 string format */
	if (is_ipv4(raw->ip)) {
		ret = snprintf(
			ip, sizeof(ip), "%u.%u.%u.%u",
			raw->ip[12], raw->ip[13], raw->ip[14], raw->ip[15]);
	} else {
		ret = snprintf(
			ip, sizeof(ip),
		        "%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x",
		        raw->ip[0], raw->ip[1], raw->ip[2], raw->ip[3],
		        raw->ip[4], raw->ip[5], raw->ip[6], raw->ip[7],
		        raw->ip[8], raw->ip[9], raw->ip[10], raw->ip[11],
		        raw->ip[12], raw->ip[13], raw->ip[14], raw->ip[15]);
	}

	assert(ret > 0 && ret < sizeof(ip));

	/* Unpack port and then write it in a string */
	portnum = (raw->port[0] << 8) | raw->port[1];
	ret = snprintf(port, sizeof(port), "%u", portnum);

	assert(ret > 0 && ret < sizeof(port));
	(void)ret; /* Don't raise a warning when assertions are disabled */

	*_ip = ip;
	*_port = port;
}

int unpack_server_addr(struct packet *packet, char **ip, char **port, int *reset_context)
{
	static size_t size;
	static unsigned char *buf;

	assert(packet != NULL);
	assert(ip != NULL);
	assert(port != NULL);
	assert(reset_context != NULL);

	if (*reset_context) {
		if (!skip_header(packet, MSG_LIST, sizeof(MSG_LIST)))
			return 0;

		size = packet->size;
		buf = packet->buffer;
		*reset_context = 0;
	}

	if (size >= sizeof(struct server_addr_raw)) {
		struct server_addr_raw raw;

		memcpy(&raw, buf, sizeof(raw));
		raw_addr_to_strings(&raw, ip, port);

		buf += sizeof(raw);
		size -= sizeof(raw);

		return 1;
	}

	return 0;
}
