#ifndef CGI_H
#define CGI_H

/* Used by pages */
#define EXIT_NOT_FOUND 2

void error(int code, char *fmt, ...);

#define MAX_DOMAIN_LENGTH 1024

extern struct cgi_config {
	const char *name;
	const char *port;

	char domain[MAX_DOMAIN_LENGTH];
} cgi_config;

int page_clan_main(int argc, char **argv);
int page_graph_main(int argc, char **argv);
int page_about_main(int argc, char **argv);
int page_player_main(int argc, char **argv);
int page_search_main(int argc, char **argv);
int page_rank_page_main(int argc, char **argv);
int page_clan_list_main(int argc, char **argv);
int page_robots_main(int argc, char **argv);
int page_sitemap_main(int argc, char **argv);

#endif /* CGI_H */
