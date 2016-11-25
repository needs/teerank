#ifndef HTML_H
#define HTML_H

#include <time.h>

#include "player.h"

void html(const char *fmt, ...);
void xml(const char *fmt, ...);
void svg(const char *fmt, ...);
void css(const char *fmt, ...);

char *escape(const char *str);

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
void player_lastseen_link(time_t _lastseen, const char *addr);

struct tab {
	char *name, *href;
};
extern const struct tab CTF_TAB, ABOUT_TAB;
extern struct tab CUSTOM_TAB;
void html_header(
	const struct tab *active, const char *title,
	const char *sprefix, const char *query);
void html_footer(const char *jsonanchor, const char *jsonurl);

/* Player list */
void html_start_player_list(int byrank, int bylastseen, unsigned pnum);
void html_end_player_list(void);
void html_player_list_entry(
	struct player *p, struct client *c, int no_clan_link);

/* Online player list */
void html_start_online_player_list(void);
void html_end_online_player_list(void);
void html_online_player_list_entry(struct player *player, struct client *client);

/* Clan list */
void html_start_clan_list(void);
void html_end_clan_list(void);
void html_clan_list_entry(
	unsigned pos, const char *hexname, unsigned nmembers);

/* Server list */
void html_start_server_list(void);
void html_end_server_list(void);
void html_server_list_entry(unsigned pos, struct server *server);

enum section_tab {
	PLAYERS_TAB, CLANS_TAB, SERVERS_TAB, SECTION_TABS_COUNT
};

void print_section_tabs(enum section_tab tab, const char *squery, unsigned *tabvals);

void print_page_nav(const char *url, unsigned pnum, unsigned npages);

#endif /* HTML_H */
