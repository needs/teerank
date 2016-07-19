#ifndef HTML_H
#define HTML_H

#include "player.h"

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
void html_footer(void);

const char *name_to_html(const char *name);

void html_start_player_list(void);
void html_player_list_entry(
	const char *hexname, const char *hexclan, int elo, unsigned rank,
	int no_clan_link);
void html_end_player_list(void);

#endif /* HTML_H */
