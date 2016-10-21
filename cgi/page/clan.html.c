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

int main_html_clan(int argc, char **argv)
{
	struct clan clan;
	unsigned missing_members, i;
	char clan_name[NAME_LENGTH];
	int ret;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <clan_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Load players */

	if (!is_valid_hexname(argv[1]))
		return EXIT_NOT_FOUND;

	if ((ret = read_clan(&clan, argv[1])) != SUCCESS)
		return ret;

	missing_members = load_members(&clan);

	/* Sort members */
	qsort(clan.members, clan.nmembers, sizeof(*clan.members), cmp_player);

	/* Eventually, print them */
	hexname_to_name(clan.name, clan_name);
	CUSTOM_TAB.name = escape(clan_name);
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, clan_name, "/clans", NULL);
	html("<h2>%s</h2>", escape(clan_name));

	if (!missing_members)
		html("<p>%u member(s)</p>", clan.nmembers);
	else
		html("<p>%u member(s), %u could not be loaded</p>",
		       clan.nmembers + missing_members, missing_members);

	html_start_player_list(1, 1, 0);

	for (i = 0; i < clan.nmembers; i++) {
		struct player_info *p = &clan.members[i];
		html_player_list_entry(
			p->name, p->clan, p->elo, p->rank, p->lastseen,
			build_addr(p->server_ip, p->server_port), 1);
	}

	html_end_player_list();

	html_footer("clan", relurl("/clans/%s.json", clan.name));

	return EXIT_SUCCESS;
}
