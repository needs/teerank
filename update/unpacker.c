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
static const uint8_t MSG_INFO_64[] = {
	255, 255, 255, 255, 'd', 't', 's', 'f'
};
static const uint8_t MSG_INFO_EXTENDED[] = {
	255, 255, 255, 255, 'i', 'e', 'x', 't'
};
static const uint8_t MSG_INFO_EXTENDED_MORE[] = {
	255, 255, 255, 255, 'i', 'e', 'x', '+'
};

struct unpacker {
	struct packet *packet;
	size_t offset;
	bool error;
};

static void init_unpacker(struct unpacker *up, struct packet *packet)
{
	assert(up != NULL);
	assert(packet != NULL);

	up->packet = packet;
	up->offset = 0;
	up->error = false;
}

static bool can_unpack(struct unpacker *up)
{
	assert(up != NULL);
	return !up->error && up->offset < up->packet->size;
}

/*
 * Unpack the actual field and set the cursor to the next field.
 * Returns an empty string on error.
 */
static char *unpack(struct unpacker *up)
{
	char *ret;

	assert(up != NULL);

	if (!can_unpack(up))
		goto fail;

	ret = (char*)&up->packet->buffer[up->offset];
	for (; up->packet->buffer[up->offset]; up->offset++)
		if (up->offset >= up->packet->size)
			goto fail;

	up->offset++;
	return ret;

fail:
	up->error = true;
	return "";
}

/*
 * It's a shame POSIX do not have a good enough function to copy strings
 * with bound check.  strncat() have its own caveat so we end up using
 * this suboptimal snprintf() trick.
 */
static void unpack_string(struct unpacker *up, char *buf, size_t size)
{
	if (snprintf(buf, size, "%s", unpack(up)) >= size)
		up->error = true;
}

/*
 * Underflow, overflow and non numeric characters set unpacker's error
 * state.  Because non-malicious packets shouldn't contain such values.
 */
static long int unpack_int(struct unpacker *up)
{
	long ret;
	char *str, *endptr;

	assert(up != NULL);

	str = unpack(up);
	errno = 0;
	ret = strtol(str, &endptr, 10);

	if (errno || !*str || *endptr)
		up->error = true;

	return ret;
}

enum packet_type {
	VANILLA, LEGACY_64, EXTENDED, EXTENDED_MORE
};

static enum packet_type packet_type(struct packet *packet)
{
	if (skip_header(packet, MSG_INFO, sizeof(MSG_INFO)))
		return VANILLA;
	if (skip_header(packet, MSG_INFO_64, sizeof(MSG_INFO_64)))
		return LEGACY_64;
	if (skip_header(packet, MSG_INFO_EXTENDED, sizeof(MSG_INFO_EXTENDED)))
		return EXTENDED;
	if (skip_header(packet, MSG_INFO_EXTENDED_MORE, sizeof(MSG_INFO_EXTENDED_MORE)))
		return EXTENDED_MORE;

	return -1;
}

enum unpack_status unpack_server_info(struct packet *packet, struct server *sv)
{
	enum packet_type type;
	struct unpacker up;
	struct client *cl;

	assert(packet != NULL);
	assert(sv != NULL);

	if ((type = packet_type(packet)) == -1)
		return UNPACK_ERROR;

	init_unpacker(&up, packet);

	/*
	 * Make things easier to read and helps identifying each fields,
	 * even the skipped ones.
	 */
	#define SKIP(var) unpack(&up)
	#define INT(var) var = unpack_int(&up)
	#define STRING(var) unpack_string(&up, var, sizeof(var))

	SKIP(token);

	if (type == EXTENDED_MORE)
		goto unpack_clients;

	SKIP(version);
	STRING(sv->name);
	STRING(sv->map);

	if (type == EXTENDED) {
		SKIP(map_crc);
		SKIP(map_size);
	}

	STRING(sv->gametype);
	SKIP(flags);
	SKIP(num_players);
	SKIP(max_players);
	INT(sv->num_clients);
	INT(sv->max_clients);

	/*
	 * We perform some sanity checks before going further to make
	 * sure the packet is well crafted.
	 */
	if (sv->num_clients > sv->max_clients)
		return UNPACK_ERROR;
	if (sv->num_clients > MAX_CLIENTS)
		return UNPACK_ERROR;
	if (sv->max_clients > MAX_CLIENTS)
		return UNPACK_ERROR;

unpack_clients:
	switch (type) {
	case VANILLA:
		break;
	case LEGACY_64:
		SKIP(offset);
		break;

	case EXTENDED_MORE:
		SKIP(pckno);
	case EXTENDED:
		SKIP(reserved);
		break;
	}

	/*
	 * We now unpack clients until there is no more clients to
	 * unpack.  Incomplete client info is considered failure, even
	 * though we could just discard it.
	 */
	while (can_unpack(&up) && (cl = new_client(sv))) {
		STRING(cl->name);
		STRING(cl->clan);
		SKIP(country);
		INT(cl->score);
		INT(cl->ingame);

		if (type == EXTENDED || type == EXTENDED_MORE)
			SKIP(reserved);
	}

	if (up.error)
		return UNPACK_ERROR;
	else if (sv->received_clients < sv->num_clients)
		return UNPACK_INCOMPLETE;
	else
		return UNPACK_SUCCESS;

	#undef SKIP
	#undef INT
	#undef STRING
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

enum unpack_status unpack_server_addr(struct packet *packet, char **ip, char **port, int *reset_context)
{
	static size_t size;
	static unsigned char *buf;

	assert(packet != NULL);
	assert(ip != NULL);
	assert(port != NULL);
	assert(reset_context != NULL);

	if (*reset_context) {
		if (!skip_header(packet, MSG_LIST, sizeof(MSG_LIST)))
			return UNPACK_ERROR;

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

		return UNPACK_SUCCESS;
	}

	return UNPACK_ERROR;
}
