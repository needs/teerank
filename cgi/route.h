#ifndef ROUTE_H
#define ROUTE_H

#define MAX_ARGS 8

struct url;
struct route;
struct route {
	const char *const name;
	const char *const content_type;
	void (*const setup)(struct route *this, struct url *url);

	int (*const main)(int argc, char **argv);
	char *args[MAX_ARGS];

	struct route *const routes;
};

struct route *do_route(char *uri, char *query);

#endif /* ROUTE_H */
