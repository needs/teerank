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

	FILE *file;
};

static void load_page(struct page *page, unsigned number)
{
	static char path[PATH_MAX];
	int ret;
	FILE *file;

	assert(page != NULL);

	ret = snprintf(path, PATH_MAX, "%s/pages/%u", config.root, number);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(path, %d): Too long\n", PATH_MAX);
		exit(EXIT_FAILURE);
	}

	if (!(file = fopen(path, "r"))) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	if (fscanf(file, "page %u/%u ", &page->number, &page->total) != 2) {
		fprintf(stderr, "%s: Cannot match page header\n", path);
		fclose(file);
		exit(EXIT_FAILURE);
	}

	page->file = file;
}

static void free_page(struct page *page)
{
	fclose(page->file);
}

static struct player_summary *next_player(struct page *page)
{
	char name[HEXNAME_LENGTH];

	while (fscanf(page->file, " %s", name) == 1) {
		static struct player_summary player;

		if (!is_valid_hexname(name))
			continue;
		if (read_player_summary(&player, name) == 0)
			continue;

		return &player;
	}

	return NULL;
}

static void print_page(struct page *page)
{
	struct player_summary *player;

	assert(page != NULL);

	while ((player = next_player(page)))
		html_print_player(player, 1);
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

	html("<nav class=\"pages\">");
	if (page->number == 1)
		html("<a class=\"previous\">Previous</a>");
	else
		html("<a class=\"previous\" href=\"/pages/%u.html\">Previous</a>",
		       page->number - 1);

	if (page->number > PAGE_SHOWN + 1)
		html("<a href=\"/pages/1.html\">1</a>");
	if (page->number > PAGE_SHOWN + 2)
		html("<span>...</span>");

	for (i = min(PAGE_SHOWN, page->number - 1); i > 0; i--)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       page->number - i, page->number - i);

	html("<a class=\"current\">%u</a>", page->number);

	for (i = 1; i <= min(PAGE_SHOWN, page->total - page->number); i++)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       page->number + i, page->number + i);

	if (page->number + PAGE_SHOWN + 1 < page->total)
		html("<span>...</span>");
	if (page->number + PAGE_SHOWN < page->total)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       page->total, page->total);

	if (page->number == page->total)
		html("<a class=\"next\">Next</a>");
	else
		html("<a class=\"next\" href=\"/pages/%u.html\">Next</a>",
		       page->number + 1);
	html("</nav>");
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
		exit(EXIT_FAILURE);
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
		exit(EXIT_NOT_FOUND);
	} else if (ret < 0 || ret > UINT_MAX) {
		fprintf(stderr, "%s: invalid page number\n", str);
		exit(EXIT_NOT_FOUND);
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
		return EXIT_FAILURE;
	}

	mode = parse_mode(argv[1]);
	page_number = parse_page_number(argv[2]);

	load_page(&page, page_number);

	if (mode == FULL_PAGE) {
		html_header(&CTF_TAB, "CTF", NULL);
		print_nav(&page);
		html_start_player_list();
	}

	print_page(&page);

	if (mode == FULL_PAGE) {
		html_end_player_list();
		print_nav(&page);
		html_footer();
	}

	free_page(&page);

	return EXIT_SUCCESS;
}
