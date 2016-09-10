#ifndef INFO_H
#define INFO_H

struct info {
	unsigned nplayers;
	unsigned nclans;
	unsigned nservers;
};

int read_info(struct info *info);
int write_info(struct info *info);

#endif /* INFO_H */
