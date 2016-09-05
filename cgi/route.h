#ifndef ROUTE_H
#define ROUTE_H

#define MAX_ARGS 8

struct url;
struct page;
typedef	void (*init_func_t)(struct page *page, struct url *url);
typedef	int (*page_func_t)(int argc, char **argv);

struct page {
	char *name;
	char *args[MAX_ARGS];

	const char *content_type;

	init_func_t init;
	page_func_t main;
};

struct page *do_route(char *uri, char *query);

#endif /* ROUTE_H */
