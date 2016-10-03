#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "delta.h"

int scan_delta(struct delta *delta)
{
	static int firstcall = 1;
	static struct jfile jfile;

	unsigned i;

	assert(delta != NULL);

	if (firstcall) {
		json_init(&jfile, stdin, "<stdin>");
		json_read_array_start(&jfile, NULL, 0);
		firstcall = 0;
	}

	if (json_try_read_array_end(&jfile))
		return 0;

	json_read_object_start(&jfile, NULL);

	json_read_string(&jfile, "ip", delta->ip, sizeof(delta->ip));
	json_read_string(&jfile, "port", delta->port, sizeof(delta->port));

	json_read_string(&jfile, "gametype", delta->gametype, sizeof(delta->gametype));
	json_read_string(&jfile, "map", delta->map, sizeof(delta->map));
	json_read_int(&jfile, "num_clients", &delta->num_clients);
	json_read_int(&jfile, "max_clients", &delta->max_clients);
	json_read_unsigned(&jfile, "length", &delta->length);
	json_read_int(&jfile, "elapsed", &delta->elapsed);

	json_read_array_start(&jfile, "players", 0);

	if (json_have_error(&jfile))
		return 0;

	for (i = 0; i < delta->length; i++) {
		struct player_delta *player = &delta->players[i];

		json_read_object_start(&jfile, NULL);
		json_read_string(&jfile, "name", player->name, sizeof(player->name));
		json_read_string(&jfile, "clan", player->clan, sizeof(player->clan));
		json_read_bool(&jfile, "ingame", &player->ingame);
		json_read_int(&jfile, "score", &player->score);
		json_read_int(&jfile, "old_score", &player->old_score);
		json_read_object_end(&jfile);

		if (json_have_error(&jfile))
			return 0;
		if (!is_valid_hexname(player->name))
			return fprintf(stderr, "<stdin>: %s: Not a valid player name\n", player->name), 0;
		if (!is_valid_hexname(player->clan))
			return fprintf(stderr, "<stdin>: %s: Not a valid player clan\n", player->clan), 0;
	}

	json_read_array_end(&jfile);
	json_read_object_end(&jfile);

	if (json_have_error(&jfile))
		return 0;

	return 1;
}

static struct jfile jstdout;

void start_printing_delta(void)
{
	json_init(&jstdout, stdout, "<stdout>");
	json_write_array_start(&jstdout, NULL, 0);
}

void stop_printing_delta(void)
{
	json_write_array_end(&jstdout);
}

int print_delta(struct delta *delta)
{
	unsigned i;

	if (!delta->length)
		return 1;

	json_write_object_start(&jstdout, NULL);

	json_write_string(&jstdout, "ip", delta->ip, sizeof(delta->ip));
	json_write_string(&jstdout, "port", delta->port, sizeof(delta->port));

	json_write_string(&jstdout, "gametype", delta->gametype, sizeof(delta->gametype));
	json_write_string(&jstdout, "map", delta->map, sizeof(delta->map));
	json_write_int(&jstdout, "num_clients", delta->num_clients);
	json_write_int(&jstdout, "max_clients", delta->max_clients);
	json_write_unsigned(&jstdout, "length", delta->length);
	json_write_int(&jstdout, "elapsed", delta->elapsed);

	json_write_array_start(&jstdout, "players", 0);

	for (i = 0; i < delta->length; i++) {
		struct player_delta *player = &delta->players[i];

		json_write_object_start(&jstdout, NULL);
		json_write_string(&jstdout, "name", player->name, sizeof(player->name));
		json_write_string(&jstdout, "clan", player->clan, sizeof(player->clan));
		json_write_bool(&jstdout, "ingame", player->ingame);
		json_write_int(&jstdout, "score", player->score);
		json_write_int(&jstdout, "old_score", player->old_score);
		json_write_object_end(&jstdout);
	}

	json_write_array_end(&jstdout);
	json_write_object_end(&jstdout);
	fflush(jstdout.file);

	return json_have_error(&jstdout);
}

static struct client *get_player(
	struct server *server, struct client *client)
{
	unsigned i;

	assert(server != NULL);
	assert(client != NULL);

	for (i = 0; i < server->num_clients; i++)
		if (strcmp(server->clients[i].name, client->name) == 0)
			return &server->clients[i];

	return NULL;
}

struct delta delta_servers(
	struct server *old, struct server *new, int elapsed)
{
	struct delta delta;
	unsigned i;

	assert(old != NULL);
	assert(new != NULL);

	strcpy(delta.ip, new->ip);
	strcpy(delta.port, new->port);

	strcpy(delta.gametype, new->gametype);
	strcpy(delta.map, new->map);

	delta.num_clients = new->num_clients;
	delta.max_clients = new->max_clients;

	delta.elapsed = elapsed;
	delta.length = new->num_clients;

	for (i = 0; i < new->num_clients; i++) {
		struct client *old_player, *new_player;
		struct player_delta *player;

		player = &delta.players[i];
		new_player = &new->clients[i];
		old_player = get_player(old, new_player);

		strcpy(player->name, new_player->name);
		strcpy(player->clan, new_player->clan);
		player->score = new_player->score;
		player->ingame = new_player->ingame;

		if (old_player)
			player->old_score = old_player->score;
		else
			player->old_score = NO_SCORE;
	}

	return delta;
}
