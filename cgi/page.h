#ifndef PAGE_H
#define PAGE_H

/*
 * Common functions used by multiple pages
 */

static const unsigned PLAYERS_PER_PAGE = 100;
static const unsigned CLANS_PER_PAGE   = 100;
static const unsigned SERVERS_PER_PAGE = 100;

/* Parse page number (used by *-list) */
int parse_pnum(const char *str, unsigned *pnum);

/* Only dump "n" json fields */
int dump_n_fields(FILE *file, unsigned n);

#endif /* PAGE_H */
