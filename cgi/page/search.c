#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#include "config.h"
#include "html.h"
#include "player.h"

/*
 * Too much results is meanless, so by limiting the number of results we can
 * statically allocate the result array.
 */
#define MAX_RESULTS 50

/*
 * Result contains the player struct because we sort result by relevance and ELO,
 * so we need to store player's data at some point.
 *
 * Results are linked together, so that way, removing or adding a result is
 * almost a O(1).
 */
struct result {
	char name[HEXNAME_LENGTH];
	int relevance;

	int is_loaded;
	struct player_info player;

	struct result *next, *prev;
};

/*
 * The pool just act as a storage for the element of the linked-list. Results
 * are sorted by their relevance.
 *
 * The pool does contain one extra result to avoid an extra copy when adding
 * a new result.  The adding process is as follow:
 *   - Get a temporary result (the extra result pointed by "free")
 *   - If it belongs to the list, just link it
 *   - If it does not, do nothing
 *
 * The list maintain a pointer to the last result so we can avoid searching
 * when the result's relevance is lower than last result relevance.  And also because
 * when the list is full and we want to add a new result, we then remove the
 * last one.
 */
struct list {
	unsigned length;
	struct result pool[MAX_RESULTS + 1];
	struct result *free;

	struct result *first;
	struct result *last;
};

static void to_lowercase(char *src, char *dst)
{
	unsigned i;

	assert(src != NULL);
	assert(dst != NULL);

	for (i = 0; src[i] != '\0'; i++)
		dst[i] = tolower(src[i]);
	dst[i] = '\0';
}

/*
 * The higher the relevance is, the better the name match the query. A relevance of
 * zero means the result will be ignored.
 */
static unsigned get_relevance(char *hex, char *query)
{
	unsigned relevance;
	char *tmp;
	char name[NAME_LENGTH];

	/* Lowercase the name to have case insensitive search */
	hexname_to_name(hex, name);
	to_lowercase(name, name);

	if (!(tmp = strstr(name, query)))
		return 0;

	relevance = 10;

	/* A query matching the beginning of a name is relevant */
	if (tmp == name)
		relevance += 20;

	/* A query matching the end of a name is relevant as well */
	if (tmp == name + strlen(name) - strlen(query))
		relevance += 15;

	/*
	 * Let's note that if a query match exactly the name, both conditions
	 * above will be matched and the relevance will be 50.
	 */
	return relevance;
}

static void init_list(struct list *list)
{
	list->length = 0;
	list->first = NULL;
	list->last = NULL;
	list->free = &list->pool[0];
}

static int is_empty(struct list *list)
{
	return list->length == 0;
}

static int is_full(struct list *list)
{
	return list->length == MAX_RESULTS;
}

/*
 * Just use the list->free pointer and initialize it.
 */
static struct result *new_result(struct list *list, unsigned relevance, char *name)
{
	list->free->relevance = relevance;
	strcpy(list->free->name, name);
	list->free->is_loaded = 0;
	return list->free;
}

/*
 * Insert 'result' before 'target'.  Remove the last result if the list is full.
 * If target is NULL, insert the result at the end of the list.
 */
static void insert_before(struct list *list, struct result *target, struct result *result)
{
	assert(list != NULL);
	assert(result != NULL);

	if (!target) {
		if (is_empty(list)) {
			/* First result in the list */
			result->prev = NULL;
			result->next = NULL;
			list->first = result;
			list->last = result;
		} else {
			/* Append the result */
			result->prev = list->last;
			result->next = NULL;
			list->last->next = result;
			list->last = result;
		}
	} else {
		/* Insert before target */
		result->prev = target->prev;
		result->next = target;
		target->prev = result;

		if (target == list->first) {
			list->first = result;
			result->prev = NULL;
		} else {
			result->prev->next = result;
		}
	}

	/* Update the list->free pointer */
	if (is_full(list)) {
		list->free = list->last;
		list->last = list->last->prev;
		list->last->next = NULL;
	} else {
		list->length++;
		list->free = &list->pool[list->length];
	}
}

static void try_load_result(struct result *result)
{
	if (!result->is_loaded) {
		enum read_player_ret ret;

		ret = read_player_info(&result->player, result->name);

		if (ret == PLAYER_FOUND)
			result->is_loaded = 1;
	}
}

static int cmp_results(struct result *a, struct result *b)
{
	if (a->relevance < b->relevance)
		return -1;
	else if (a->relevance > b->relevance)
		return 1;

	try_load_result(a);
	try_load_result(b);

	if (a->player.rank > b->player.rank)
		return -1;
	else if (a->player.rank < b->player.rank)
		return 1;

	return 0;
}

static void try_add_result(struct list *list, unsigned relevance, char *name)
{
	struct result *result, *r;

	assert(list != NULL);

	if (relevance == 0)
		return;

	result = new_result(list, relevance, name);

	if (is_empty(list))
		return insert_before(list, NULL, result);

	/*
	 * We know the list is not empty and therefor list->last is set.
	 * If the list is not full just add the result at the end.
	 */
	if (cmp_results(list->last, result) > 0) {
		if (!is_full(list))
			insert_before(list, NULL, result);
		return;
	}

	/*
	 * Insert the result in a sorted manner
	 */
	for (r = list->first; r; r = r->next)
		if (cmp_results(r, result) < 0)
			break;
	insert_before(list, r, result);
}

static int search(char *query, struct list *list)
{
	DIR *dir;
	struct dirent *dp;
	static char path[PATH_MAX];
	char lowercase_query[NAME_LENGTH];

	assert(strlen(query) < NAME_LENGTH);

	if (snprintf(path, PATH_MAX, "%s/players", config.root) >= PATH_MAX) {
		fprintf(stderr, "Path to teerank database too long\n");
		return 0;
	}

	if (!(dir = opendir(path))) {
		fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
		return 0;
	}

	to_lowercase(query, lowercase_query);
	init_list(list);

	while ((dp = readdir(dir))) {
		unsigned relevance;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		relevance = get_relevance(dp->d_name, lowercase_query);
		try_add_result(list, relevance, dp->d_name);
	}

	closedir(dir);
	return 1;
}

static struct list LIST_ZERO;

int page_search_main(int argc, char **argv)
{
	struct list list = LIST_ZERO;
	size_t length;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* No need to search when the query is too long or empty */
	length = strlen(argv[1]);
	if (length > 0 && length < NAME_LENGTH) {
		if (!search(argv[1], &list))
			return EXIT_FAILURE;
	}

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, "Search results", argv[1]);

	if (list.length == 0) {
		html("No players found");
	} else {
		struct result *result;

		html_start_player_list();
		for (result = list.first; result; result = result->next) {
			struct player_info *p;

			try_load_result(result);

			p = &result->player;
			html_player_list_entry(p->name, p->clan, p->elo, p->rank, 0);
		}
		html_end_player_list();
	}

	html_footer();

	return EXIT_SUCCESS;
}
