#ifndef CONFIG_H
#define CONFIG_H

struct config {
	char *root;
	char *cache_root;
	char *tmp_root;
	short verbose;
};

extern struct config config;

void load_config(void);
void verbose(const char *fmt, ...);

#endif /* CONFIG_H */
