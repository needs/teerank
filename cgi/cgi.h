#ifndef CGI_H
#define CGI_H

/* Used by pages */
#define EXIT_NOT_FOUND 2

void error(int code, char *fmt, ...);

int page_clan_main(int argc, char **argv);
int page_graph_main(int argc, char **argv);
int page_about_main(int argc, char **argv);
int page_player_main(int argc, char **argv);
int page_search_main(int argc, char **argv);
int page_rank_page_main(int argc, char **argv);

#endif /* CGI_H */
