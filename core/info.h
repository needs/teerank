#ifndef INFO_H
#define INFO_H

#include <time.h>

struct info {
	unsigned nplayers;
	unsigned nclans;
	unsigned nservers;
	struct tm last_update;
};

int read_info(struct info *info);
int write_info(struct info *info);

#endif /* INFO_H */
