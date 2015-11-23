#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <sys/stat.h>

#define MAX_PLAYERS 16
struct delta {
	time_t elapsed;
	unsigned length;
	struct player {
		char *name, *clan;
		long delta;
		long score;
		long elo;
	} players[MAX_PLAYERS];
};

static int read_delta(struct delta *delta)
{
	unsigned i;
	int ret;

	assert(delta != NULL);

	ret = scanf(" %u %ld", &delta->length, &delta->elapsed);
	if (ret == EOF)
		return 0;
	assert(ret == 2);

	for (i = 0; i < delta->length; i++) {
		ret = scanf(" %ms %ms %ld %ld",
		            &delta->players[i].name,
		            &delta->players[i].clan,
		            &delta->players[i].score,
		            &delta->players[i].delta);
		assert(ret == 4);
	}

	return 1;
}

/* Prefix is set in main() */
static char *prefix;

static char *get_path(char *player, char *suffix)
{
	static char path[PATH_MAX];

	assert(strlen(prefix) + strlen(player) + strlen(suffix) + 2 < PATH_MAX);

	sprintf(path, "%s/%s/%s", prefix, player, suffix);
	return path;
}

static int read_elo(struct player *player)
{
	FILE *file;
	char *path;

	assert(player != NULL);

	path = get_path(player->name, "elo");

	file = fopen(path, "r");
	if (!file) {
		if (errno == ENOENT) {
			player->elo = 1500;
			return 1;
		}
		perror(path);
		return 0;
	}

	if (fscanf(file, "%ld", &player->elo) == 1)
		return fclose(file), 1;
	else
		return fclose(file), 0;
}

static int write_elo(struct player *player)
{
	FILE *file;
	char *path;

	assert(player != NULL);

	path = get_path(player->name, "elo");

	if (!(file = fopen(path, "w"))) {
		perror(path);
		return 0;
	}

	fprintf(file, "%ld", player->elo);
	fclose(file);
	return 1;
}

/*
 * p() func as defined by ELO.
 */
static double p(double delta)
{
	if (delta > 400.0)
		delta = 400.0;
	else if (delta < -400.0)
		delta = -400.0;

	return 1.0 / (1.0 + pow(10.0, -delta / 400.0));
}

/* Classic ELO formula for two players */
static int compute_elo_delta(struct player *player, struct player *opponent)
{
	static const unsigned K = 10;
	double W;

	assert(player != NULL);
	assert(opponent != NULL);
	assert(player != opponent);

	if (player->delta < opponent->delta)
		W = 0.0;
	else if (player->delta == opponent->delta)
		W = 0.5;
	else
		W = 1.0;

	return K * (W - p(player->elo - opponent->elo));
}

/*
 * ELO has been designed for two players games only.  In order to have
 * meaningul value for multiplayer, we have to modify how ELO score is
 * computed.
 *
 * To get the new ELO for a player, we match this player against every
 * other player and each time we add the ELO delta.  The final ELO delta
 * is then added to the player's ELO to get his new ELO score.
 */
static int sum_elo_delta(struct delta *delta, struct player *player)
{
	unsigned i;
	int elo_delta = 0;

	assert(delta != NULL);
	assert(player != NULL);

	for (i = 0; i < delta->length; i++) {
		struct player *opponent = &delta->players[i];

		if (opponent != player)
			elo_delta += compute_elo_delta(player, opponent);
	}

	return elo_delta;
}

static int load_elos(struct delta *delta)
{
	unsigned i;

	assert(delta != NULL);

	if (!delta->length)
		return 1;

	for (i = 0; i < delta->length; i++)
		if (!read_elo(&delta->players[i]))
			return 0;

	return 1;
}

static int create_players_directory(struct delta *delta)
{
	unsigned i;
	char *path;

	assert(delta != NULL);

	for (i = 0; i < delta->length; i++) {
		path = get_path(delta->players[i].name, "");

		if (mkdir(path, S_IRWXU) == -1) {
			if (errno != EEXIST) {
				perror(path);
				return 0;
			}
		}
	}

	return 1;
}

static void update(struct delta *delta)
{
	unsigned i;

	assert(delta != NULL);

	if (!create_players_directory(delta))
		return;
	if (!load_elos(delta))
		return;

	printf("Elapsed = %ld\n", delta->elapsed);
	for (i = 0; i < delta->length; i++) {
		struct player *player;

		player = &delta->players[i];
		printf("Player: \"%s\",\tDelta: %ld,\tbefore: %ld,\t",
		       player->name, player->delta, player->elo);
		player->elo += sum_elo_delta(delta, player);
		printf("after: %ld\n", player->elo);
		write_elo(player);
	}
	putchar('\n');
}

static void remove_unrankable_players(struct delta *delta)
{
	unsigned i;

	assert(delta != NULL);

	for (i = 0; i < delta->length; i++) {
		struct player *player = &delta->players[i];

		if (player->score <= 0 || player->delta < -5) {
			delta->players[i] = delta->players[delta->length - 1];
			delta->length--;
			i--;
		}
	}
}

static unsigned is_rankable(struct delta *delta)
{
	assert(delta != NULL);

	/* At least 2 minute should have passed since the last update,
	 * otherwise the score delta is not meaningful enough. */
	if (delta->elapsed < 3 * 60)
		return 0;

	/* We need at least 4 players really playing the game (ie. rankable)
	 * to get meaningful ELO deltas. */
	remove_unrankable_players(delta);
	if (delta->length < 4)
		return 0;

	return 1;
}

int main(int argc, char **argv)
{
	struct delta delta;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	prefix = argv[1];

	while (read_delta(&delta))
		if (is_rankable(&delta))
			update(&delta);

	return EXIT_SUCCESS;
}
