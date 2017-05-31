#ifndef IO_H
#define IO_H

void reset_output(void);
void print_output(void);

void html(const char *fmt, ...);
void xml(const char *fmt, ...);
void svg(const char *fmt, ...);
void css(const char *fmt, ...);
void json(const char *fmt, ...);
void txt(const char *fmt, ...);
char *URL(const char *fmt, ...);

#endif /* IO_H */
