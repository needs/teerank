#ifndef HTML_H
#define HTML_H

#include <time.h>

#include "player.h"
#include "index.h"

void html(const char *fmt, ...);
void xml(const char *fmt, ...);
void svg(const char *fmt, ...);
void css(const char *fmt, ...);

char *escape(const char *str);

struct tab {
	char *name, *href;
};
extern const struct tab CTF_TAB, ABOUT_TAB;
extern struct tab CUSTOM_TAB;
void html_header(const struct tab *active, char *title, char *search);
void html_footer(const char *jsonanchor);

const char *name_to_html(const char *name);

/* Player list */
void html_start_player_list(int byrank, int bylastseen);
void html_end_player_list(void);
void html_player_list_entry(
	const char *hexname, const char *hexclan,
	int elo, unsigned rank, struct tm lastseen,
	int no_clan_link);

/* Clan list */
void html_start_clan_list(void);
void html_end_clan_list(void);
void html_clan_list_entry(
	unsigned pos, const char *hexname, unsigned nmembers);

/* Server list */
void html_start_server_list(void);
void html_end_server_list(void);
void html_server_list_entry(unsigned pos, struct indexed_server *server);

enum section_tab {
	PLAYERS_TAB, CLANS_TAB, SERVERS_TAB
};

void print_section_tabs(enum section_tab tab, const char *squery, unsigned num);

void print_page_nav(const char *url, struct index_page *ipage);

#endif /* HTML_H */
