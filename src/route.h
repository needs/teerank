#ifndef ROUTE_H
#define ROUTE_H

struct route {
	char *cache;
	char *source;
	char **args;
};

struct route *do_route(char *path, char *query);

#endif /* ROUTE_H */
