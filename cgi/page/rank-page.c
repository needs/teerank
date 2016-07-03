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
	unsigned pnum, npages;
	unsigned i;

	FILE *file;
};

#define PLAYERS_PER_PAGE 100

static void load_page(struct page *page, unsigned pnum)
{
	static char path[PATH_MAX];
	unsigned nplayers, npages;
	int ret;
	FILE *file;

	assert(page != NULL);

	ret = snprintf(path, PATH_MAX, "%s/ranks", config.root);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "snprintf(path, %d): Too long\n", PATH_MAX);
		exit(EXIT_FAILURE);
	}

	if (!(file = fopen(path, "r"))) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	/* First, read header */
	if (fscanf(file, "%u players\n", &nplayers) != 1) {
		fprintf(stderr, "%s: No header\n", path);
		exit(EXIT_FAILURE);
	}

	npages = nplayers / PLAYERS_PER_PAGE + 1;
	if (pnum > npages) {
		fprintf(stderr, "Only %u pages available\n", npages);
		exit(EXIT_NOT_FOUND);
	}

	if (fseek(file, HEXNAME_LENGTH * (pnum - 1) * PLAYERS_PER_PAGE, SEEK_CUR) == -1) {
		perror("fseek()");
		exit(EXIT_FAILURE);
	}

	page->npages = npages;
	page->pnum = pnum;
	page->file = file;
}

static void free_page(struct page *page)
{
	fclose(page->file);
}

static struct player_summary *next_player(struct page *page)
{
	char name[HEXNAME_LENGTH];

	while (page->i < PLAYERS_PER_PAGE) {
		static struct player_summary player;

		if (fscanf(page->file, " %s", name) != 1)
			break;

		page->i++;

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
	unsigned pnum, npages, i;

	assert(page != NULL);

	pnum = page->pnum;
	npages = page->npages;

	html("<nav class=\"pages\">");
	if (pnum == 1)
		html("<a class=\"previous\">Previous</a>");
	else
		html("<a class=\"previous\" href=\"/pages/%u.html\">Previous</a>",
		       pnum - 1);

	if (pnum > PAGE_SHOWN + 1)
		html("<a href=\"/pages/1.html\">1</a>");
	if (pnum > PAGE_SHOWN + 2)
		html("<span>...</span>");

	for (i = min(PAGE_SHOWN, pnum - 1); i > 0; i--)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       pnum - i, pnum - i);

	html("<a class=\"current\">%u</a>", pnum);

	for (i = 1; i <= min(PAGE_SHOWN, npages - pnum); i++)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       pnum + i, pnum + i);

	if (pnum + PAGE_SHOWN + 1 < npages)
		html("<span>...</span>");
	if (pnum + PAGE_SHOWN < npages)
		html("<a href=\"/pages/%u.html\">%u</a>",
		       npages, npages);

	if (pnum == npages)
		html("<a class=\"next\">Next</a>");
	else
		html("<a class=\"next\" href=\"/pages/%u.html\">Next</a>",
		       pnum + 1);
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
	} else if (ret == 0) {
		fprintf(stderr, "%s: Pages start at 1\n", str);
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
