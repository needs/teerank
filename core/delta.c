#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "delta.h"

int scan_delta(struct delta *delta)
{
	unsigned i;
	int ret;

	assert(delta != NULL);

	errno = 0;
	ret = scanf(" %u %d", &delta->length, &delta->elapsed);
	if (ret == EOF && errno == 0)
		return 0;
	else if (ret == EOF && errno != 0)
		return perror("<stdin>"), 0;
	else if (ret == 0)
		return fprintf(stderr, "<stdin>: Cannot match delta length\n"), 0;
	else if (ret == 1)
		return fprintf(stderr, "<stdin>: Cannot match delta elapsed time\n"), 0;

	for (i = 0; i < delta->length; i++) {
		errno = 0;
		ret = scanf(" %s %s %d %d",
		            delta->players[i].name,
		            delta->players[i].clan,
		            &delta->players[i].score,
		            &delta->players[i].delta);
		if (ret == EOF && errno == 0)
			return fprintf(stderr, "<stdin>: Expected %u players, found %u\n", delta->length, i), 0;
		else if (ret == EOF && errno != 0)
			return perror("<stdin>"), 0;
		else if (ret == 0)
			return fprintf(stderr, "<stdin>: Cannot match player name\n"), 0;
		else if (ret == 1)
			return fprintf(stderr, "<stdin>: Cannot match player clan\n"), 0;
		else if (ret == 2)
			return fprintf(stderr, "<stdin>: Cannot match player score\n"), 0;
		else if (ret == 3)
			return fprintf(stderr, "<stdin>: Cannot match player delta\n"), 0;

		if (!is_valid_hexname(delta->players[i].name))
			return fprintf(stderr, "<stdin>: %s: Not a valid player name\n", delta->players[i].name), 0;
		if (!is_valid_hexname(delta->players[i].clan))
			return fprintf(stderr, "<stdin>: %s: Not a valid player clan\n", delta->players[i].clan), 0;
	}

	return 1;
}

void print_delta(struct delta *delta)
{
	if (delta->length) {
		unsigned i;

		printf("%u %d\n", delta->length, delta->elapsed);

		for (i = 0; i < delta->length; i++) {
			printf("%s %s %d %d\n",
			       delta->players[i].name,
			       delta->players[i].clan,
			       delta->players[i].score,
			       delta->players[i].delta);
		}
	}
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

	delta.elapsed = elapsed;
	delta.length = 0;
	for (i = 0; i < new->num_clients; i++) {
		struct client *old_player, *new_player;

		new_player = &new->clients[i];
		old_player = get_player(old, new_player);

		if (old_player) {
			struct player_delta *player;
			player = &delta.players[delta.length];
			strcpy(player->name, new_player->name);
			strcpy(player->clan, new_player->clan);
			player->score = new_player->score;
			player->delta = new_player->score - old_player->score;
			delta.length++;
		}
	}

	return delta;
}
