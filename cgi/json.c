#include <stdio.h>
#include <time.h>

#include "json.h"

char *json_escape(const char *str)
{
	static char buf[1024], *c;

	for (c = buf; *str && c - buf < sizeof(buf) - 1; str++, c++) {
		if (*str == '"') {
			*c = '\\';
			c++;
		}
		*c = *str;
	}

	*c = '\0';

	return buf;
}

char *json_date(time_t t)
{
	/* RFC-3339 */
	static char buf[] = "yyyy-mm-ddThh:mm:ssZ";
	size_t ret;

	ret = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
	if (ret != sizeof(buf) - 1) {
		fprintf(stderr, "Cannot convert to RFC-3339 time string");
		return "1970-00-00T00:00:00Z";
	}

	return buf;
}

const char *json_boolean(int boolean)
{
	if (boolean)
		return "true";
	else
		return "false";
}
