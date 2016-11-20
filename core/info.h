#ifndef INFO_H
#define INFO_H

#include <time.h>

#define MAX_MASTERS 8

struct master_info {
	char node[sizeof("masterX.teeworlds.com")];
	char service[sizeof("8300")];

	time_t lastseen;
	unsigned nservers;
};

struct info {
	unsigned nplayers;
	unsigned nclans;
	unsigned nservers;
	time_t last_update;

	unsigned nmasters;
	struct master_info masters[MAX_MASTERS];
};

int read_info(struct info *info);

#endif /* INFO_H */
