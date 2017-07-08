#ifndef IO_H
#define IO_H

/*
 * Those functions use a subset of printf() conversion specifiers, and
 * may add some for dates and booleans for instance.  They also encode
 * user provided string if the target format requires it.
 */
void html(const char *fmt, ...);
void xml(const char *fmt, ...);
void svg(const char *fmt, ...);
void css(const char *fmt, ...);
void json(const char *fmt, ...);
void txt(const char *fmt, ...);

/*
 * The previously declared functions don't actually directly print on
 * stdout but instead store the result in a buffer.  The following
 * functions does reset this buffer or dump it.
 */
void reset_output(void);
void print_output(void);

/*
 * For convenience, URL() store its result in the provided buffer.  To
 * keep things simple and consistent, we provide the following typedef
 * so that the maximum size is always the same and can be queried with
 * sizeof(url_t).
 */
typedef char url_t[256];

/*
 * URL() parameters list must be NULL-terminated, so we add this macro
 * to always NULL-terminate the list and avoid headaches when debugging
 * a missing NULL.
 */
void URL_(url_t buf, const char *prefix, ...);
#define URL(buf, prefix, ...) URL_(buf, prefix, ##__VA_ARGS__, NULL)

#define PARAM(name, dflt, type, val)                                    \
	name, dflt, type, val
#define ANCHOR(val)                                                     \
	"", val
#define PARAM_NAME(val)                                                 \
	PARAM("name", NULL, "%s", val)
#define PARAM_IP(val)                                                   \
	PARAM("ip", NULL, "%s", val)
#define PARAM_PORT(val)                                                 \
	PARAM("port", "8303", "%s", val)
#define PARAM_GAMETYPE(val)                                             \
	PARAM("gametype", "CTF", "%s", val)
#define PARAM_MAP(val)                                                  \
	PARAM("map", "", "%s", val)
#define PARAM_PAGENUM(val)                                              \
	PARAM("p", "1", "%u", val)
#define PARAM_ORDER(val)                                                \
	PARAM("sort", NULL, "%s", val)
#define PARAM_SQUERY(val)                                               \
	PARAM("q", "", "%s", val)

/* Extract a paramater in an URL */
#define URL_EXTRACT(url, ...)                                           \
	URL_EXTRACT_(url, __VA_ARGS__)
#define URL_EXTRACT_(url, name, dflt, type, val)                        \
	URL_EXTRACT__(url, name, dflt)

/* Default parameter value */
#define DEFAULT_PARAM_VALUE(param) DEFAULT_PARAM_VALUE_(param)
#define DEFAULT_PARAM_VALUE_(name, dflt, type, val) dflt

struct url;
char *URL_EXTRACT__(struct url *url, char *name, char *dflt);

/*
 * Lists is a concept shared by both HTML and JSON page, here we define
 * a structure holding list info used by both formats as well as
 * functions to initialize this structure.
 */
struct list {
	sqlite3_stmt *res;
	unsigned nrow;
	unsigned pnum;
	unsigned plen;

	char *order;
	char *ordername;
};

struct list_order {
	char *name;
	char *orderby;
};

struct list init_list(
	const char *qselect, const char *qcount,
	unsigned plen, unsigned pnum, struct list_order *orders, char *order,
	const char *bindfmt, ...);
struct list init_simple_list(const char *qselect, const char *bindfmt, ...);

#endif /* IO_H */
