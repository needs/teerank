#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>

#include <netinet/in.h>

#include "network.h"
#include "pool.h"
#include "delta.h"
#include "io.h"
#include "config.h"

static const unsigned char MSG_GETINFO[] = {
	255, 255, 255, 255, 'g', 'i', 'e', '3'
};
static const unsigned char MSG_INFO[] = {
	255, 255, 255, 255, 'i', 'n', 'f', '3'
};

struct unpacker {
	struct data *data;
	size_t offset;
};

static int init_unpacker(struct unpacker *up, struct data *data)
{
	assert(up != NULL);
	assert(data != NULL);

	up->data = data;
	up->offset = 0;

	/*
	 * If the last byte is not 0, then it will overflow when unpacking
	 * a string.
	 */
	return data->buffer[data->size - 1] == 0;
}

static int can_unpack(struct unpacker *up, unsigned length)
{
	unsigned offset;

	assert(up != NULL);
	assert(length > 0);

	for (offset = up->offset; offset < up->data->size; offset++)
		if (up->data->buffer[offset] == 0)
			if (--length == 0)
				return 1;

	fprintf(stderr, "can_unpack(): %u field(s) missing\n", length);
	return 0;
}

static char *unpack_string(struct unpacker *up)
{
	size_t old_offset;

	assert(up != NULL);

	old_offset = up->offset;
	while (up->offset < up->data->size
	       && up->data->buffer[up->offset] != 0)
		up->offset++;

	/* Skip the remaining 0 */
	up->offset++;

	/* can_unpack() should have been used */
	assert(up->offset <= up->data->size);

	return (char*)&up->data->buffer[old_offset];
}

static long int unpack_int(struct unpacker *up)
{
	long ret;
	char *str, *endptr;

	assert(up != NULL);

	str = unpack_string(up);
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

#define MAX_CLIENTS 16

struct server_info {
	char *gametype;

	int num_clients;
	struct client {
		char name[MAX_NAME_LENGTH], clan[MAX_NAME_LENGTH];
		long int score;
		long int ingame;
	} clients[MAX_CLIENTS];

	time_t time;
};

static int validate_client_info(struct client *client)
{
	char tmp[MAX_NAME_LENGTH];

	assert(client != NULL);
	assert(client->name != NULL);
	assert(client->clan != NULL);

	if (strlen(client->name) > 16)
		return 0;
	if (strlen(client->clan) > 16)
		return 0;

	string_to_hex(client->name, tmp);
	strcpy(client->name, tmp);
	string_to_hex(client->clan, tmp);
	strcpy(client->clan, tmp);

	return 1;
}

static int validate_clients_info(struct server_info *info)
{
	unsigned i;

	assert(info != NULL);

	for (i = 0; i < info->num_clients; i++)
		if (!validate_client_info(&info->clients[i]))
			return 0;
	return 1;
}

static int unpack_server_info(struct data *data, struct server_info *info)
{
	struct unpacker up;
	unsigned i;

	assert(data != NULL);
	assert(info != NULL);

	/* Unpack server info (ignore useless infos) */
	if (!init_unpacker(&up, data))
		return 0;
	if (!can_unpack(&up, 10))
		return 0;

	unpack_string(&up);     /* Token */
	unpack_string(&up);     /* Version */
	unpack_string(&up);     /* Name */
	unpack_string(&up);     /* Map */
	info->gametype = unpack_string(&up);     /* Gametype */

	if (strcmp(info->gametype, "CTF"))
		return 0;

	unpack_string(&up);     /* Flags */
	unpack_string(&up);     /* Player number */
	unpack_string(&up);     /* Player max number */
	info->num_clients = unpack_int(&up);     /* Client number */
	unpack_string(&up);     /* Client max number */

	if (info->num_clients > MAX_CLIENTS) {
		fprintf(stderr, "max_clients shouldn't be higher than %d\n",
		        MAX_CLIENTS);
		return 0;
	}

	/* Players */
	for (i = 0; i < info->num_clients; i++) {
		if (!can_unpack(&up, 5))
			return 0;

		strcpy(info->clients[i].name, unpack_string(&up)); /* Name */
		strcpy(info->clients[i].clan, unpack_string(&up)); /* Clan */
		unpack_string(&up); /* Country */
		info->clients[i].score  = unpack_int(&up); /* Score */
		info->clients[i].ingame = unpack_int(&up); /* Ingame? */
	}

	if (up.offset != up.data->size) {
		fprintf(stderr,
		        "%lu bytes remaining after unpacking server info\n",
		        up.data->size - up.offset);
	}

	if (!validate_clients_info(info))
		return 0;

	return 1;
}

struct server {
	char dirname[PATH_MAX];
	struct sockaddr_storage addr;
	struct pool_entry entry;
};

struct server_list {
	unsigned length;
	struct server *servers;
};

static int read_server_info(FILE *file, char *path, struct server_info *info)
{
	unsigned i;
	int ret;

	assert(file != NULL);
	assert(path != NULL);
	assert(info != NULL);

	ret = fscanf(file, "%d", &info->num_clients);

	if (ferror(file)) {
		perror(path);
		return 0;
	} else if (ret == EOF) {
		info->gametype = NULL;
		return 1;
	} else if (ret == 0) {
		fprintf(stderr, "%s: Cannot match clients number\n", path);
		return 0;
	}

	info->gametype = "CTF";
	for (i = 0; i < info->num_clients; i++) {
		struct client *client = &info->clients[i];

		ret = fscanf(file, " %s %s %ld",
		             client->name, client->clan, &client->score);
		if (ferror(file)) {
			perror(path);
			return 0;
		} else if (ret == EOF) {
			fprintf(stderr, "%s: Early end-of-file\n", path);
			return 0;
		} else if (ret < 3) {
			fprintf(stderr, "%s: Only %d elements matched\n", path, ret);
			return 0;
		}
	}

	return 1;
}

static int write_server_info(FILE *file, char *path, struct server_info *info)
{
	unsigned i;

	assert(file != NULL);
	assert(info != NULL);

	if (info->num_clients == 0)
		return 1;
	if (fprintf(file, "%d\n", info->num_clients) <= 0)
		return perror(path), 0;

	for (i = 0; i < info->num_clients; i++) {
		struct client *client = &info->clients[i];

		if (fprintf(file, "%s %s %ld\n", client->name, client->clan, client->score) <= 0)
			return perror(path), 0;
	}

	return 1;
}

static char *get_path(const char *dirname, const char *filename)
{
	static char path[PATH_MAX];

	assert(strlen(dirname) + 1 + strlen(filename) < PATH_MAX);
	sprintf(path, "%s/%s", dirname, filename);

	return path;
}

static int dump_server_info(
	struct server *server,
	struct server_info *old, struct server_info *new)
{
	static const char filename[] = "infos";
	struct stat stats;
	char *path;
	FILE *file;
	int ret;

	assert(server != NULL);
	assert(old != NULL);
	assert(new != NULL);
	assert(!strcmp(new->gametype, "CTF"));

	path = get_path(server->dirname, filename);

	/* Read old state (if any) */
	ret = stat(path, &stats);
	if (ret == 0) {
		if (!(file = fopen(path, "r")))
			return perror(path), 0;
		if (!read_server_info(file, path, old))
			return fclose(file), 0;
		fclose(file);
		old->time = stats.st_mtim.tv_sec;
	} else if (errno == ENOENT) {
		old->gametype = NULL;
	} else {
		return perror(path), 0;
	}

	/* Write new state */
	if (!(file = fopen(path, "w")))
		return perror(path), 0;
	if (!write_server_info(file, path, new))
		return fclose(file), 0;
	fclose(file);

	if (stat(path, &stats) == -1)
		return perror(path), 0;
	new->time = stats.st_mtim.tv_sec;

	return 1;
}

static struct client *get_player(
	struct server_info *info, struct client *client)
{
	unsigned i;

	assert(info != NULL);
	assert(client != NULL);

	for (i = 0; i < info->num_clients; i++)
		if (!strcmp(info->clients[i].name, client->name))
			return &info->clients[i];

	return NULL;
}

static void print_server_info_delta(
	struct server_info *old, struct server_info *new)
{
	struct delta delta;
	unsigned i;

	assert(old != NULL);
	assert(new != NULL);
	assert(!strcmp(new->gametype, "CTF"));

	/* A NULL gametype means empty old server infos */
	if (!old->gametype)
		return;
	assert(!strcmp(old->gametype, "CTF"));

	delta.elapsed = new->time - old->time;
	delta.length = 0;
	for (i = 0; i < new->num_clients; i++) {
		struct client *old_player, *new_player;

		new_player = &new->clients[i];
		old_player = get_player(old, new_player);

		if (old_player) {
			struct player_delta *player;
			player = &delta.players[delta.length];
			player->name = new_player->name;
			player->clan = new_player->clan;
			player->score = new_player->score;
			player->delta = new_player->score - old_player->score;
			delta.length++;
		}
	}

	print_delta(&delta);
}

static void remove_spectators(struct server_info *info)
{
	unsigned i;

	assert(info != NULL);

	for (i = 0; i < info->num_clients; i++) {
		if (!info->clients[i].ingame) {
			info->clients[i] = info->clients[--info->num_clients];
			i--;
		}
	}
}

static int handle_data(struct data *data, struct server *server)
{
	struct server_info old, new;

	assert(data != NULL);
	assert(server != NULL);

	if (!skip_header(data, MSG_INFO, sizeof(MSG_INFO)))
		return 0;
	if (!unpack_server_info(data, &new))
		return 0;

	remove_spectators(&new);
	if (!dump_server_info(server, &old, &new))
		return 0;

	print_server_info_delta(&old, &new);

	return 1;
}

static void restore_pathname(char *pathname, unsigned tokens)
{
	unsigned i;

	/* Restore dirname content altered by strtok() */
	for (i = 1; i < tokens; i++)
		*strchr(pathname, '\0') = ' ';
}

static int is_valid_tokens(char *version, char *ip, char *port, char *pathname)
{
	unsigned tokens;

	assert(pathname != NULL);

	tokens = !!version + !!ip + !!port;

	if (!version) {
		restore_pathname(pathname, tokens);
		fprintf(stderr, "%s: Cannot find IP version (v4 or v6)\n", pathname);
		return 0;
	} else if (!ip) {
		restore_pathname(pathname, tokens);
		fprintf(stderr, "%s: Cannot find IP\n", pathname);
		return 0;
	} else if (!port) {
		restore_pathname(pathname, tokens);
		fprintf(stderr, "%s: Cannot find port\n", pathname);
		return 0;
	}

	/* Check version */
	if (strcmp(version, "v4") && strcmp(version, "v6")) {
		restore_pathname(pathname, tokens);
		fprintf(stderr, "%s: IP version should be either v4 or v6\n", pathname);
		return 0;
	}

	/* IP and port are implicitely checked later in get_sockaddr() */
	return 1;
}

static int extract_ip_and_port(char *pathname, char *_ip, char *_port)
{
	char c;
	unsigned i;
	char *version, *ip, *port;

	assert(pathname != NULL);
	assert(_ip != NULL);
	assert(_port != NULL);

	version = strtok(basename(pathname), " ");
	ip = strtok(NULL, " ");
	port = strtok(NULL, " ");

	if (!is_valid_tokens(version, ip, port, pathname))
		return 0;

	/* Copy tokens so we can modify them without altering pathname */
	strcpy(_ip, ip);
	strcpy(_port, port);

	/* Replace '_' by '.' or ':' depending on IP version */
	if (version[1] == '4')
		c = '.';
	else
		c = ':';
	for (i = 0; i < strlen(_ip); i++)
		if (_ip[i] == '_')
			_ip[i] = c;

	/* Revert the effect of strtok() */
	restore_pathname(pathname, 3);

	return 1;
}

static int add_server(struct server_list *list, struct server *server)
{
	static const unsigned OFFSET = 1024;

	if (list->length % OFFSET == 0) {
		struct server *servers;
		servers = realloc(list->servers, sizeof(*servers) * (list->length + OFFSET));
		if (!servers)
			return 0;
		list->servers = servers;
	}

	list->servers[list->length++] = *server;
	return 1;
}

static int fill_server_list(struct server_list *list)
{
	DIR *dir;
	struct dirent *dp;
	char path[PATH_MAX];

	assert(list != NULL);

	sprintf(path, "%s/servers", config.root);
	if (!(dir = opendir(path)))
		return perror(path), 0;

	/* Fill array (ignore server on error) */
	while ((dp = readdir(dir))) {
		char ip[IP_LENGTH + 1], port[PORT_LENGTH + 1];
		struct server server;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		sprintf(server.dirname, "%s/%s", path, dp->d_name);
		if (!extract_ip_and_port(dp->d_name, ip, port))
			continue;
		if (!get_sockaddr(ip, port, &server.addr))
			continue;
		if (!add_server(list, &server))
			continue;
	}

	closedir(dir);

	return 1;
}

static struct server *get_server(struct pool_entry *entry)
{
	assert(entry != NULL);
	return (struct server*)((char*)entry - offsetof(struct server, entry));
}

static void poll_servers(struct server_list *list, struct sockets *sockets)
{
	const struct data request = {
		sizeof(MSG_GETINFO) + 1, {
			MSG_GETINFO[0], MSG_GETINFO[1], MSG_GETINFO[2],
			MSG_GETINFO[3], MSG_GETINFO[4], MSG_GETINFO[5],
			MSG_GETINFO[6], MSG_GETINFO[7], 0
		}
	};
	struct pool pool;
	struct pool_entry *entry;
	struct data answer;
	unsigned i;

	assert(list != NULL);
	assert(sockets != NULL);

	init_pool(&pool, sockets, &request);
	for (i = 0; i < list->length; i++)
		add_pool_entry(&pool, &list->servers[i].entry,
		               &list->servers[i].addr);

	while ((entry = poll_pool(&pool, &answer)))
		handle_data(&answer, get_server(entry));
}

static const struct server_list SERVER_LIST_ZERO;

int main(int argc, char **argv)
{
	struct sockets sockets;
	struct server_list list = SERVER_LIST_ZERO;

	load_config();
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!fill_server_list(&list))
		return EXIT_FAILURE;

	if (!init_sockets(&sockets))
		return EXIT_FAILURE;
	poll_servers(&list, &sockets);
	close_sockets(&sockets);

	return EXIT_SUCCESS;
}
