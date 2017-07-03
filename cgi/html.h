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
void player_lastseen(time_t lastseen);

enum tab_type {
	CTF_TAB,
	DM_TAB,
	TDM_TAB,
	GAMETYPE_TAB,
	GAMETYPE_LIST_TAB,
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
	HTML_COLTYPE_PLAYER_COUNT,
	HTML_COLTYPE_MAP,
	HTML_COLTYPE_GAMETYPE
};

struct html_list_column {
	char *title;
	char *order;
	enum html_coltype type;
};

typedef char *(*row_class_func_t)(sqlite3_stmt *res);
struct html_list_args {
	char *order;
	char *class;
	char *url;
	row_class_func_t row_class;
};
void html_list_(
	struct list *list, struct html_list_column *cols, struct html_list_args args);
#define html_list(list, cols, ...)                                       \
	html_list_(list, cols, (struct html_list_args){ __VA_ARGS__ })

struct section_tab {
	char *title;
	url_t url;
	unsigned val;
	bool active;
};

void print_section_tabs(struct section_tab *tabs);

#endif /* HTML_H */
