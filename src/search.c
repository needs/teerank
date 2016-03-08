#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "config.h"
#include "io.h"
#include "player.h"

/*
 * Too much results is meanless, so by limiting the number of results we can
 * statically allocate the result array.
 */
#define MAX_RESULTS 50

/*
 * Results are linked together, so that way, removing or adding a result is
 * almost a O(1).
 */
struct result {
	char name[MAX_NAME_LENGTH];
	int score;

	struct result *next, *prev;
};

/*
 * The pool just act as a storage for the element of the linked-list. Results
 * are sorted by their score.
 *
 * The list maintain a pointer to the last result so we can avoid searching
 * when the result's score is lower than last result score.  And also because
 * when the list is full and we want to add a new result, we then remove the
 * last one.
 */
struct list {
	unsigned length;
	struct result pool[MAX_RESULTS];
	struct result *first;
	struct result *last;
};

/*
 * The higher the score is, the better the name match the query. A score of
 * zero means the result will be ignored.
 */
static unsigned get_score(char *name, char *query)
{
	unsigned score;
	char *tmp;

	if (!(tmp = strstr(name, query)))
		return 0;

	/*
	 * Since name and query are in hexadecimal, each charcaters is two
	 * bytes wide, so make sure we haven't matched two overlapping chars.
	 */
	if ((tmp - name) % 2 == 1)
		return 0;

	score = 10;

	/* A query matching the beginning of a name is relevant */
	if (tmp == name)
		score += 20;

	/* A query matching the end of a name is relevant as well */
	if (tmp == name + strlen(name) - strlen(query) - 2)
		score += 15;

	/*
	 * Let's not that if a query match exactly the name, both conditions
	 * above will be matched and the score will be 50.
	 */
	return score;
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
 * Take a result from the pool if the pool is not full, otherwise remove
 * the last result from the list, reset it's content and return it.
 */
static struct result *new_result(struct list *list, unsigned score, char *name)
{
	struct result *result;

	if (!is_full(list)) {
		/* Just return the next result from the pool */
		result = &list->pool[list->length++];
	} else {
		/* The list is full, free the last entry and return it */
		result = list->last;
		list->last = result->prev;

		result->prev->next = NULL;
		result->prev = NULL;
		result->next = NULL;
	}

	result->score = score;
	strcpy(result->name, name);
	return result;
}

/*
 * Insert 'result' before 'target'.  If target is NULL, add the result at the
 * end of the list.
 */
static void insert_before(struct list *list, struct result *target, struct result *result)
{
	assert(list != NULL);
	assert(result != NULL);

	if (target == list->first)
		list->first = result;

	if (target) {
		result->prev = target->prev;
		result->next = target;
		if (target->prev)
			target->prev->next = result;
		target->prev = result;
	} else {
		/* Insert at the end */
		result->prev = list->last;
		result->next = NULL;
		if (list->last)
			list->last->next = result;
		list->last = result;
	}
}

static void try_add_result(struct list *list, unsigned score, char *name)
{
	struct result *result;

	assert(list != NULL);

	if (score == 0)
		return;

	if (is_empty(list))
		return insert_before(list, NULL, new_result(list, score, name));

	/*
	 * We know the list is not empty and therefor list->last is set.
	 * If the list is not full just add the result at the end.
	 */
	if (score < list->last->score) {
		if (is_full(list))
			return;
		else
			return insert_before(list, NULL, new_result(list, score, name));
	}

	/*
	 * At this point we know the result should be in the list, but we have
	 * to compute where to place it.
	 */
	for (result = list->first; result; result = result->next)
		if (result->score <= score)
			return insert_before(list, result, new_result(list, score, name));
}

static void search(char *hex, struct list *list)
{
	DIR *dir;
	struct dirent *dp;
	static char path[PATH_MAX];

	if (snprintf(path, PATH_MAX, "%s/players", config.root) >= PATH_MAX) {
		fprintf(stderr, "Path to teerank database too long\n");
		exit(EXIT_FAILURE);
	}

	if (!(dir = opendir(path))) {
		fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while ((dp = readdir(dir))) {
		unsigned score;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		score = get_score(dp->d_name, hex);
		try_add_result(list, score, dp->d_name);
	}

	closedir(dir);
}

static struct list LIST_ZERO;

int main(int argc, char **argv)
{
	static char hex[MAX_NAME_LENGTH];
	struct list list = LIST_ZERO;
	size_t length;

	load_config();

	if (argc != 2) {
		fprintf(stderr, "usage: %s <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* No need to search when the query is too long or empty */
	length = strlen(argv[1]);
	if (length > 0 && length < MAX_NAME_LENGTH) {
		string_to_hex(argv[1], hex);
		hex[strlen(hex) - 2] = '\0';
		search(hex, &list);
	}

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";
	print_header(&CUSTOM_TAB, "Search results", argv[1]);

	if (list.length == 0) {
		printf("No players found");
	} else {
		struct result *result;

		printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");
		for (result = list.first; result; result = result->next) {
			struct player player;
			read_player(&player, result->name);
			html_print_player(&player, 1);
		}
		printf("</tbody></table>");
	}

	print_footer();

	return EXIT_SUCCESS;
}
