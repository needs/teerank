#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "player.h"
#include "clan.h"

static const struct clan CLAN_ZERO;

/*
 * Remove "player" from "src_clan" and add it to "dest_clan".
 *
 * This function does the best to make sure if one step fail, the database
 * remain in a consistent state.
 */
static int clan_move_player(char *src_clan, char *dest_clan, char *player)
{
	int ret;

	assert(src_clan != NULL);
	assert(dest_clan != NULL);
	assert(player != NULL);
	assert(strcmp(src_clan, dest_clan));

	/*
	 * Add the player first to make sure the player is referenced by
	 * at least one clan at any time.
	 */
	if (strcmp(dest_clan, "00"))
		if (!add_member_inline(dest_clan, player))
			return 0;

	if (strcmp(src_clan, "00")) {
		struct clan clan = CLAN_ZERO;

		if (read_clan(&clan, src_clan) != CLAN_FOUND)
			return 0;

		remove_member(&clan, get_member(&clan, player));
		ret = write_clan(&clan);
		free_clan(&clan);

		if (!ret)
			return 0;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	int ret;
	char player[HEXNAME_LENGTH];
	char    old[HEXNAME_LENGTH];
	char    new[HEXNAME_LENGTH];

	load_config(1);

	while ((ret = scanf(" %s %s %s", player, old, new)) == 3) {
		if (!strcmp(old, new)) {
			fprintf(stderr, "scanf(delta): Old and new clan must be different (%s)\n", old);
			continue;
		} else if (!is_valid_hexname(old)) {
			fprintf(stderr, "scanf(delta): Expected old clan in hexadecimal form (%s)\n", old);
			continue;
		} else if (!is_valid_hexname(new)) {
			fprintf(stderr, "scanf(delta): Expected new clan in hexadecimal form (%s)\n", new);
			continue;
		}
		clan_move_player(old, new, player);
	}

	if (ret == EOF && !ferror(stdin))
		return EXIT_SUCCESS;
	else if (ferror(stdin))
		perror("scanf(delta)");
	else if (ret == 0)
		fprintf(stderr, "scanf(delta): Cannot match player name\n");
	else if (ret == 1)
		fprintf(stderr, "scanf(delta): Cannot match old clan\n");
	else if (ret == 2)
		fprintf(stderr, "scanf(delta): Cannot match new clan\n");

	return EXIT_FAILURE;
}
