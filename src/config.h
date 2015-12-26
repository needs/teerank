#ifndef CONFIG_H
#define CONFIG_H

struct config {
	char *root;
	char *cache_root;
	char *tmp_root;
};

extern struct config config;

void load_config(void);

#endif /* CONFIG_H */
