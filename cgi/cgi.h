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

#define PLAYERS_PER_PAGE 100;
#define CLANS_PER_PAGE   100;
#define SERVERS_PER_PAGE 100;

/* PCS stands for Player Clan Server */
enum pcs {
	PCS_PLAYER,
	PCS_CLAN,
	PCS_SERVER
};

int parse_pnum(const char *str, unsigned *pnum);

int parse_list_args(
	int argc, char **argv,
	enum pcs *pcs, char **gametype, char **map, char **order, unsigned *pnum);

unsigned char hextodec(char c);

char *url_encode(const char *str);
void url_decode(char *str);

char *relurl(const char *fmt, ...);
char *absurl(const char *fmt, ...);

int main_html_status(int argc, char **argv);

int main_html_about(int argc, char **argv);
int main_json_about(int argc, char **argv);

int main_html_about_json_api(int argc, char **argv);
int main_html_search(int argc, char **argv);
int main_svg_graph(int argc, char **argv);

int main_html_player(int argc, char **argv);
int main_json_player(int argc, char **argv);

int main_html_clan(int argc, char **argv);
int main_json_clan(int argc, char **argv);

int main_html_server(int argc, char **argv);
int main_json_server(int argc, char **argv);

int main_html_list(int argc, char **argv);
int main_json_player_list(int argc, char **argv);
int main_json_clan_list(int argc, char **argv);
int main_json_server_list(int argc, char **argv);

int main_txt_robots(int argc, char **argv);
int main_xml_sitemap(int argc, char **argv);

#endif /* CGI_H */
