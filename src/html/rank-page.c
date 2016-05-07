#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>

#include "config.h"
#include "html.h"
#include "player.h"

struct page {
	unsigned number, total;

	struct player_array players;
};

static void load_page(struct page *page, unsigned number)
{
	char name[HEXNAME_LENGTH];
	static char path[PATH_MAX];
	int ret;
	FILE *file;

	assert(page != NULL);

	ret = snprintf(path, PATH_MAX, "%s/pages/%u", config.root, number);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(path, %d): Too long\n", PATH_MAX);
		exit(500);
	}

	if (!(file = fopen(path, "r"))) {
		perror(path);
		exit(500);
	}

	if (fscanf(file, "page %u/%u ", &page->number, &page->total) != 2) {
		fprintf(stderr, "%s: Cannot match page header\n", path);
		fclose(file);
		exit(500);
	}

	while (fscanf(file, " %s", name) == 1) {
		struct player player = PLAYER_ZERO;
		if (!is_valid_hexname(name))
			continue;
		if (read_player(&player, name, 0))
			add_player(&page->players, &player);
	}

	fclose(file);
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

enum mode {
	FULL_PAGE, ONLY_ROWS
};

static enum mode parse_mode(const char *str)
{
	assert(str != NULL);

	if (!strcmp(str, "full-page"))
		return FULL_PAGE;
	else if (!strcmp(str, "only-rows"))
		return ONLY_ROWS;
	else {
		fprintf(stderr, "First argument must be either \"full-page\" or \"only-rows\"\n");
		exit(500);
	}
}

static unsigned parse_page_number(const char *str)
{
	long ret;
	char *endptr;

	assert(str != NULL);

	errno = 0;
	ret = strtol(str, &endptr, 10);
	if (ret == 0 && errno == EINVAL) {
		fprintf(stderr, "%s: is not a number\n", str);
		exit(404);
	} else if (ret < 0 || ret > UINT_MAX) {
		fprintf(stderr, "%s: invalid page number\n", str);
		exit(404);
	}

	return (unsigned)ret;
}

static const struct page PAGE_ZERO;

int main(int argc, char **argv)
{
	struct page page = PAGE_ZERO;
	enum mode mode;
	unsigned page_number;

	load_config();
	if (argc != 3) {
		fprintf(stderr, "usage: %s [full-page|only-rows] <page_number>\n", argv[0]);
		return 500;
	}

	mode = parse_mode(argv[1]);
	page_number = parse_page_number(argv[2]);

	load_page(&page, page_number);

	if (mode == FULL_PAGE) {
		html_header(&CTF_TAB, "CTF", NULL);
		print_nav(&page);

		printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");
	}

	print_page(&page);

	if (mode == FULL_PAGE) {
		printf("</tbody></table>");
		print_nav(&page);
		html_footer();
	}

	return EXIT_SUCCESS;
}
