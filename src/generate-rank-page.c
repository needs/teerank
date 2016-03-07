#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>

#include "io.h"
#include "config.h"
#include "player.h"

struct page {
	unsigned number, total;

	struct player_array players;
};

static void load_page(struct page *page)
{
	char name[MAX_NAME_LENGTH];

	assert(page != NULL);

	if (scanf("page %u/%u ", &page->number, &page->total) != 2) {
		fprintf(stderr, "Cannot match page header\n");
		exit(EXIT_FAILURE);
	}

	while (scanf(" %s", name) == 1) {
		struct player player;
		read_player(&player, name);
		add_player(&page->players, &player);
	}
}

static void print_page(struct page *page)
{
	unsigned i;

	assert(page != NULL);

	for (i = 0; i < page->players.length; i++)
		html_print_player(&page->players.players[i], 1);
}

static unsigned min(unsigned a, unsigned b)
{
	return (a < b) ? a : b;
}

static void print_nav(struct page *page)
{
	/* Number of pages shown before and after the current page */
	static const unsigned PAGE_SHOWN = 3;
	unsigned i;

	assert(page != NULL);

	printf("<nav class=\"pages\">");
	if (page->number == 1)
		printf("<a class=\"previous\">Previous</a>");
	else
		printf("<a class=\"previous\" href=\"/pages/%u.html\">Previous</a>",
		       page->number - 1);

	if (page->number > PAGE_SHOWN + 1)
		printf("<a href=\"/pages/1.html\">1</a>");
	if (page->number > PAGE_SHOWN + 2)
		printf("<span>...</span>");

	for (i = min(PAGE_SHOWN, page->number - 1); i > 0; i--)
		printf("<a href=\"/pages/%u.html\">%u</a>",
		       page->number - i, page->number - i);

	printf("<a class=\"current\">%u</a>", page->number);

	for (i = 1; i <= min(PAGE_SHOWN, page->total - page->number); i++)
		printf("<a href=\"/pages/%u.html\">%u</a>",
		       page->number + i, page->number + i);

	if (page->number + PAGE_SHOWN + 1 < page->total)
		printf("<span>...</span>");
	if (page->number + PAGE_SHOWN < page->total)
		printf("<a href=\"/pages/%u.html\">%u</a>",
		       page->total, page->total);

	if (page->number == page->total)
	printf("<a class=\"next\">Next</a>");
	else
		printf("<a class=\"next\" href=\"/pages/%u.html\">Next</a>",
		       page->number + 1);
	printf("</nav>");
}

static const struct page PAGE_ZERO;
enum mode {
	FULL_PAGE, ONLY_ROWS
};

int main(int argc, char **argv)
{
	struct page page = PAGE_ZERO;
	enum mode mode;

	load_config();
	if (argc != 2) {
		fprintf(stderr, "usage: %s [full-page|only-rows]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!strcmp(argv[1], "full-page"))
		mode = FULL_PAGE;
	else if (!strcmp(argv[1], "only-rows"))
		mode = ONLY_ROWS;
	else {
		fprintf(stderr, "First argument must be either \"full-page\" or \"only-rows\"\n");
		return EXIT_FAILURE;
	}

	load_page(&page);

	if (mode == FULL_PAGE) {
		print_header(&CTF_TAB, "CTF", NULL);
		print_nav(&page);

		printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");
	}

	print_page(&page);

	if (mode == FULL_PAGE) {
		printf("</tbody></table>");
		print_nav(&page);
		print_footer();
	}

	return EXIT_SUCCESS;
}
