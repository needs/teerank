#ifndef ROUTE_H
#define ROUTE_H

struct route {
	char *cache_path;
	char *cache_location;
	char **args;
};

struct route *do_route(char *path, char *query);

#endif /* ROUTE_H */
