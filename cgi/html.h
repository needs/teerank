#ifndef HTML_H
#define HTML_H

#include <time.h>
#include <stdbool.h>

#include "database.h"
#include "io.h"

/*
 * Compute the number of minutes, hours, days, months and years from the
 * given date to now.  Set the most significant timescale and return a
 * number of time elapsed in "timescale" unit.
 *
 * If "text" is not NULL, store a string representation of the elapsed time.
 */
unsigned elapsed_time(time_t t, char **timescale, char *text, size_t textsize);

/*
 * Print a link to the given server if player is online.  Otherwise it
 * print the elapsed time since it's last connection.
 */
void player_lastseen_link(time_t lastseen, char *ip, char *port);

enum tab_type {
	CTF_TAB,
	DM_TAB,
	TDM_TAB,
	GAMETYPE_TAB,
	CUSTOM_TAB,
	ABOUT_TAB
};

struct html_header_args {
	char *title;
	char *subtab;
	char *subtab_url;
	char *subtab_class;

	char *search_url;
	char *search_query;
};

void html_header_(
	enum tab_type active, struct html_header_args args);
#define html_header(active, ...)                                        \
	html_header_(active, (struct html_header_args){ __VA_ARGS__ })

void html_footer(char *jsonanchor, char *jsonurl);

enum html_coltype {
	HTML_COLTYPE_NONE,
	HTML_COLTYPE_RANK,
	HTML_COLTYPE_ELO,
	HTML_COLTYPE_PLAYER,
	HTML_COLTYPE_ONLINE_PLAYER,
	HTML_COLTYPE_CLAN,
	HTML_COLTYPE_SERVER,
	HTML_COLTYPE_LASTSEEN,
	HTML_COLTYPE_PLAYER_COUNT
};

struct html_list_column {
	char *title;
	char *order;
	enum html_coltype type;
};

typedef char *(*list_class_func_t)(sqlite3_stmt *res);

/* Almost every list class function will look like this */
#define DEFINE_SIMPLE_LIST_CLASS_FUNC(name, class)                      \
static char *name(sqlite3_stmt *res)                                    \
{                                                                       \
	if (!res)                                                       \
		return class;                                           \
	else                                                            \
		return NULL;                                            \
}

void html_list(
	sqlite3_stmt *res, struct html_list_column *cols, char *order,
	list_class_func_t class, url_t url, unsigned pnum, unsigned nrow);

struct section_tab {
	char *title;
	url_t url;
	unsigned val;
	bool active;
};

void print_section_tabs(struct section_tab *tabs);

#endif /* HTML_H */
