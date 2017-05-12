#ifndef ROUTE_H
#define ROUTE_H

#define MAX_DIRS 8
#define MAX_ARGS 8

struct arg {
	char *name;
	char *val;
};

struct url {
	unsigned ndirs;
	char *dirs[MAX_DIRS];

	unsigned nargs;
	struct arg args[MAX_ARGS];
};

struct route;
struct route {
	const char *const name, *const ext;
	const char *const content_type;

	int (*const main)(struct url *url);

	struct route *const routes;
};

struct route *do_route(struct url *url);

#endif /* ROUTE_H */
