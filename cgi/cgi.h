#ifndef CGI_H
#define CGI_H

#include "route.h"
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

/* PCS stands for Player Clan Server */
enum pcs {
	PCS_PLAYER,
	PCS_CLAN,
	PCS_SERVER
};

int parse_pnum(char *str, unsigned *pnum);

unsigned char hextodec(char c);

char *url_encode(const char *str);
void url_decode(char *str);

char *relurl(const char *fmt, ...);
char *absurl(const char *fmt, ...);

int main_html_status(struct url *url);

int main_html_about(struct url *url);
int main_json_about(struct url *url);

int main_html_about_json_api(struct url *url);
int main_html_search(struct url *url);
int main_svg_graph(struct url *url);

int main_html_player(struct url *url);
int main_json_player(struct url *url);

int main_html_clan(struct url *url);
int main_json_clan(struct url *url);

int main_html_server(struct url *url);
int main_json_server(struct url *url);

int main_html_list(struct url *url);
int main_json_list(struct url *url);

int main_txt_robots(struct url *url);
int main_xml_sitemap(struct url *url);

#endif /* CGI_H */
