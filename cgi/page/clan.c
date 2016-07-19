#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "cgi.h"
#include "config.h"
#include "html.h"
#include "player.h"
#include "clan.h"

static int cmp_player(const void *p1, const void *p2)
{
	const struct player *a = p1, *b = p2;

	/* We want them in reverse order */
	if (b->rank > a->rank)
		return -1;
	if (b->rank < a->rank)
		return 1;
	return 0;
}

int page_clan_main(int argc, char **argv)
{
	struct clan clan;
	unsigned missing_members, i;
	char clan_name[NAME_LENGTH];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Load players */

	if (!is_valid_hexname(argv[1]))
		return EXIT_NOT_FOUND;
	if (!read_clan(&clan, argv[1]))
		return EXIT_NOT_FOUND;
	missing_members = load_members(&clan);

	/* Sort members */
	qsort(clan.members, clan.length, sizeof(*clan.members), cmp_player);

	/* Eventually, print them */
	hexname_to_name(clan.name, clan_name);
	html_header(&CTF_TAB, clan_name, NULL);
	html("<h2>%s</h2>", clan_name);

	if (!missing_members)
		html("<p>%u member(s)</p>", clan.length);
	else
		html("<p>%u member(s), %u could not be loaded</p>",
		       clan.length + missing_members, missing_members);

	html_start_player_list();

	for (i = 0; i < clan.length; i++) {
		struct player_summary *p = &clan.members[i];
		html_player_list_entry(p->name, p->clan, p->elo, p->rank, 1);
	}

	html_end_player_list();
	html_footer();

	return EXIT_SUCCESS;
}
