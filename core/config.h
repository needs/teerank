#ifndef CONFIG_H
#define CONFIG_H

#define EXIT_NOT_FOUND 2

struct config {
	char *root;
	short verbose;
	short debug;
};

extern struct config config;

void load_config(void);
void verbose(const char *fmt, ...);

#endif /* CONFIG_H */
