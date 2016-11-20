#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "delta.h"

int scan_delta(struct delta *delta)
{
	assert(delta != NULL);

	if (fread(delta, sizeof(*delta), 1, stdin) != 1) {
		if (ferror(stdin))
			perror("<stdin>: scan_delta()");
		return 0;
	}

	return 1;
}

int print_delta(struct delta *delta)
{
	assert(delta != NULL);

	if (fwrite(delta, sizeof(*delta), 1, stdout) != 1 && ferror(stdin)) {
		perror("<stdin>: print_delta()");
		return 0;
	}

	return 1;
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
