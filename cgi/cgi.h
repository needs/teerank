#ifndef CGI_H
#define CGI_H

#include <stdlib.h>

/* Used by pages */
#define EXIT_NOT_FOUND 2

void error(int code, char *fmt, ...);
void redirect(const char *fmt, ...);

#define MAX_DOMAIN_LENGTH 1024

extern struct cgi_config {
	const char *name;
	const char *port;

	char domain[MAX_DOMAIN_LENGTH];
} cgi_config;

int page_about_html_main(int argc, char **argv);
int page_about_json_main(int argc, char **argv);

int page_about_json_api_main(int argc, char **argv);
int page_search_main(int argc, char **argv);
int page_graph_main(int argc, char **argv);

int page_player_html_main(int argc, char **argv);
int page_player_json_main(int argc, char **argv);

int page_clan_html_main(int argc, char **argv);
int page_clan_json_main(int argc, char **argv);

int page_server_html_main(int argc, char **argv);
int page_server_json_main(int argc, char **argv);

int page_player_list_html_main(int argc, char **argv);
int page_player_list_json_main(int argc, char **argv);

int page_clan_list_html_main(int argc, char **argv);
int page_clan_list_json_main(int argc, char **argv);

int page_server_list_html_main(int argc, char **argv);
int page_server_list_json_main(int argc, char **argv);

int page_robots_main(int argc, char **argv);
int page_sitemap_main(int argc, char **argv);

#endif /* CGI_H */
