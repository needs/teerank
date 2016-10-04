#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#include "config.h"
#include "cgi.h"
#include "html.h"
#include "player.h"
#include "index.h"

/*
 * Too much results is meaningless, and by limiting the number of
 * results we can statically allocate the result array.
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
	unsigned relevance;
	void *data;
	struct result *next, *prev;
};

/*
 * The pool just act as a storage for the element of the linked-list.  Results
 * are sorted by their relevance.
 *
 * The pool contain one extra result to avoid an extra copy when adding
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
	struct index_page ipage;

	unsigned length;
	struct result pool[MAX_RESULTS + 1];
	struct result *free;

	struct result *first;
	struct result *last;
};

static void free_list(struct list *list)
{
	close_index_page(&list->ipage);
}

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
 * The higher the relevance is, the better the name match the query. A
 * relevance of zero means the result will be ignored.
 */
static unsigned name_relevance(const char *query, char *name)
{
	unsigned relevance;
	char *tmp;

	/* Lowercase the name to have case insensitive search */
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

static unsigned player_relevance(const char *query, void *data)
{
	struct indexed_player *player = data;
	char name[NAME_LENGTH];

	hexname_to_name(player->name, name);
	return name_relevance(query, name);
}

static unsigned clan_relevance(const char *query, void *data)
{
	struct indexed_clan *clan = data;
	char name[NAME_LENGTH];

	hexname_to_name(clan->name, name);
	return name_relevance(query, name);
}

static unsigned server_relevance(const char *query, void *data)
{
	struct indexed_server *server = data;
	unsigned relname, reladdr;

	relname = name_relevance(query, server->name);
	reladdr = name_relevance(query, build_addr(server->ip, server->port));

	return relname > reladdr ? relname : reladdr;
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
 * List->free always point to a free, ready to be used result.
 */
static struct result *new_result(struct list *list)
{
	return list->free;
}

/*
 * Insert 'result' before 'target'.  Remove the last result if the list
 * is full.  If target is NULL, insert the result at the end of the
 * list.  Finally, make sure the "free" pointer still point to an
 * unallocated result.
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

typedef int (*cmp_func_t)(const void *pa, const void *pb);

static int cmp_players(const void *pa, const void *pb)
{
	const struct indexed_player *a = pa, *b = pb;

	if (a->rank > b->rank)
		return -1;
	else if (a->rank < b->rank)
		return 1;

	return 0;
}

static int cmp_clans(const void *pa, const void *pb)
{
	const struct indexed_clan *a = pa, *b = pb;

	if (a->nmembers > b->nmembers)
		return 1;
	else if (a->nmembers < b->nmembers)
		return -1;

	return 0;
}

static int cmp_servers(const void *pa, const void *pb)
{
	const struct indexed_server *a = pa, *b = pb;

	if (a->nplayers > b->nplayers)
		return 1;
	else if (a->nplayers < b->nplayers)
		return -1;

	return 0;
}

static int cmp_results(
	const struct result *a, const struct result *b, cmp_func_t cmp)
{
	if (a->relevance < b->relevance)
		return -1;
	else if (a->relevance > b->relevance)
		return 1;

	return cmp(&a->data, &b->data);
}

static void try_add_result(
	struct list *list, struct result *result, cmp_func_t cmp, int onlycount)
{
	struct result *r;

	assert(list != NULL);

	if (result->relevance == 0)
		return;

	if (onlycount) {
		list->length++;
		return;
	}

	if (is_empty(list))
		return insert_before(list, NULL, result);

	/*
	 * Before checking all available results, we compare our result
	 * against the last one, that way it can already be ruled out
	 * when it is of lower relevance.
	 */
	if (cmp_results(list->last, result, cmp) > 0) {
		if (!is_full(list))
			insert_before(list, NULL, result);
		return;
	}

	/*
	 * From the last comparison, we know the result belongs to the
	 * list, we just check all results to find where it has to be
	 * inserted.
	 */
	for (r = list->first; r; r = r->next)
		if (cmp_results(r, result, cmp) < 0)
			break;
	insert_before(list, r, result);
}

static void start_player_list(void)
{
	html_start_player_list(1, 1, 0);
}

static void print_player(unsigned pos, void *data)
{
	struct indexed_player *p = data;
	html_player_list_entry(
		p->name, p->clan, p->elo, p->rank, *gmtime(&p->lastseen),
		build_addr(p->server_ip, p->server_port), 0);
}
static void print_clan(unsigned pos, void *data)
{
	struct indexed_clan *clan = data;
	html_clan_list_entry(pos, clan->name, clan->nmembers);
}
static void print_server(unsigned pos, void *data)
{
	html_server_list_entry(pos, data);
}

const struct search_info {
	void (*start_list)(void);
	void (*end_list)(void);
	void (*print_result)(unsigned pos, void *data);
	unsigned (*relevance)(const char *query, void *data);
	cmp_func_t compare;
	const char *emptylist;

	const struct index_data_info *datainfo;
	const char *indexname;

	enum section_tab tab;
	const char *sprefix;
} PLAYER_SINFO = {
	start_player_list,
	html_end_player_list,
	print_player,
	player_relevance,
	cmp_players,
	"No players found",

	&INDEX_DATA_INFO_PLAYER,
	"players_by_rank",

	PLAYERS_TAB,
	"/players"
}, CLAN_SINFO = {
	html_start_clan_list,
	html_end_clan_list,
	print_clan,
	clan_relevance,
	cmp_clans,
	"No clans found",

	&INDEX_DATA_INFO_CLAN,
	"clans_by_nmembers",

	CLANS_TAB,
	"/clans"
}, SERVER_SINFO = {
	html_start_server_list,
	html_end_server_list,
	print_server,
	server_relevance,
	cmp_servers,
	"No servers found",

	&INDEX_DATA_INFO_SERVER,
	"servers_by_nplayers",

	SERVERS_TAB,
	"/servers"
};

static int search(
	const struct search_info *sinfo, char *query, struct list *list,
	int onlycount)
{
	struct result *result;

	char lquery[ADDR_STRSIZE];
	size_t length;
	int ret;

	/* No need to search when the query is too long or empty */
	length = strlen(query);
	if (length == 0 || length >= sizeof(lquery))
		return SUCCESS;

	to_lowercase(query, lquery);
	init_list(list);

	ret = open_index_page(
		sinfo->indexname, &list->ipage, sinfo->datainfo, 1, 0);
	if (ret != SUCCESS)
		return ret;

	goto start;
	while ((result->data = index_page_foreach(&list->ipage, NULL))) {
		result->relevance = sinfo->relevance(lquery, result->data);
		try_add_result(list, result, sinfo->compare, onlycount);
	start:
		result = new_result(list);
	}

	return SUCCESS;
}

static struct list LIST_ZERO;

int page_search_main(int argc, char **argv)
{
	const struct search_info *sinfo, **sinfos = (const struct search_info*[]) {
		&PLAYER_SINFO, &CLAN_SINFO, &SERVER_SINFO, NULL
	};
	struct list list = LIST_ZERO;
	int ret;
	unsigned tabvals[SECTION_TABS_COUNT];

	if (argc != 3) {
		fprintf(stderr, "usage: %s players|clans|servers <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "players") == 0)
		sinfo = &PLAYER_SINFO;
	else if (strcmp(argv[1], "clans") == 0)
		sinfo = &CLAN_SINFO;
	else if (strcmp(argv[1], "servers") == 0)
		sinfo = &SERVER_SINFO;
	else {
		fprintf(stderr, "%s: Should be either \"players\", \"clans\" or \"servers\"\n", argv[1]);
		return EXIT_FAILURE;
	}

	/*
	 * Search in every data, but only keep results for data pointed
	 * by sinfo.
	 */

	if ((ret = search(sinfo, argv[2], &list, 0)) != SUCCESS)
		return ret;

	while (*sinfos) {
		if (*sinfos != sinfo) {
			struct list tmp = LIST_ZERO;

			if (search(*sinfos, argv[2], &tmp, 1) == SUCCESS)
				tabvals[(*sinfos)->tab] = tmp.length;

			free_list(&tmp);
		} else {
			tabvals[(*sinfos)->tab] = list.length;
		}
		sinfos++;
	}

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, "Search results", sinfo->sprefix, argv[2]);
	print_section_tabs(sinfo->tab, argv[2], tabvals);

	if (list.length == 0) {
		html("%s", sinfo->emptylist);
	} else {
		struct result *r;
		unsigned i;

		sinfo->start_list();
		for (r = list.first, i = 0; r; r = r->next, i++)
			sinfo->print_result(i + 1, r->data);
		sinfo->end_list();
	}

	html_footer(NULL);

	free_list(&list);

	return EXIT_SUCCESS;
}
