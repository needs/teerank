#ifndef JSON_H
#define JSON_H

char *json_escape(const char *string);
char *json_date(time_t t);
const char *json_boolean(int boolean);

#endif /* JSON_H */
