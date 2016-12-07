#ifndef JSON_H
#define JSON_H

char *json_escape(const char *string);
char *json_date(time_t t);
const char *json_boolean(int boolean);
char *json_hexstring(char *str);

#endif /* JSON_H */
