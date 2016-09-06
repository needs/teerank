#ifndef PAGE_H
#define PAGE_H

/*
 * Common functions used by multiple pages
 */

/* Parse page number (used by *-list) */
int parse_pnum(const char *str, unsigned *pnum);

static const unsigned PLAYERS_PER_PAGE = 100;
static const unsigned CLANS_PER_PAGE   = 100;
static const unsigned SERVERS_PER_PAGE = 100;

#endif /* PAGE_H */
