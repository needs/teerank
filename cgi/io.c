#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "cgi.h"

/*
 * "dynbuf" stands for dynamic buffer, and store and unbounded amount of
 * data in memory (if "fixed" isn't set).  We then provide a function to
 * dump the content on stdout or reset it.  It is used by html()-like
 * functions to buffer output, waiting for an eventual error.
 */
struct dynbuf {
	bool havedata;
	char *data;
	size_t size;
	size_t pos;
	bool fixed;
};

/*
 * Dynamic buffers can optionally use an already allocated buffer.  In
 * such case it won't try to reallocate it and just fail right away.
 */
static void reset_dynbuf(struct dynbuf *buf, char *dbuf, size_t size)
{
	buf->havedata = false;
	buf->pos = 0;

	if (dbuf) {
		buf->data = dbuf;
		buf->size = size;
		buf->fixed = true;
	}
}

static void free_dynbuf(struct dynbuf *buf)
{
	if (buf->data && !buf->fixed) {
		free(buf->data);
		buf->data = NULL;
		buf->size = 0;
	}
}

/*
 * Grow buffer internal memory on demand.  On failure return a 500 but
 * keep the old buffer in place so that a new attempt will be made the
 * next time a page is generated.
 */
static void bputc(struct dynbuf *buf, char c)
{
	if (buf->pos == buf->size) {
		size_t newsize;
		char *newbuf;

		if (buf->fixed)
			error(500, "Buffer is full (%sz bytes)", buf->size);

		newsize = buf->size + 10 * 1024;
		newbuf = realloc(buf->data, newsize);

		if (!newbuf)
			error(
				500, "Reallocating dynamic buffer (%sz bytes): %s",
				newsize, strerror(errno));

		buf->data = newbuf;
		buf->size = newsize;
	}

	buf->havedata = true;
	buf->data[buf->pos++] = c;
}

static void bputs(struct dynbuf *buf, const char *str)
{
	if (str) {
		buf->havedata = true;
		for (; *str; str++)
			bputc(buf, *str);
	}
}

static void bprint(struct dynbuf *buf, const char *fmt, ...)
{
	char tmp[16];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);
	bputs(buf, tmp);
}

static void nul_terminate(struct dynbuf *buf)
{
	if (buf->havedata)
		bputc(buf, '\0');
}

/*
 * It turns out we can't pass a pointer to a va_list in functions.  It
 * does raise an error because nothing prevent va_list to be an array,
 * wich don't decay to a pointer when passing its adress in a function,
 * raising a type error.
 *
 * To make it work and still allow a function to use a va_list and
 * having the caller use the modified va_list, we need to encapsulate it
 * in a structure.
 */
typedef struct va_list__ { va_list ap; } *va_list_;

#define va_start_(ap, var) do {                                         \
	ap = &(struct va_list__){};                                     \
	va_start(ap->ap, var);                                          \
} while(0)
#define va_arg_(ap, type)                                               \
	va_arg(ap->ap, type)
#define va_end_(ap)                                                     \
	va_end(ap->ap)

/* Escape callback is called for user provided strings signaled by "%s" */
typedef void (*escape_func_t)(struct dynbuf *buf, const char *str);

/* Extend callback is used to extend the set of conversion specifiers */
typedef bool (*extend_func_t)(struct dynbuf *buf, char c, va_list_ ap);

static void print(struct dynbuf *buf, escape_func_t escape, extend_func_t extend, const char *fmt, va_list_ ap)
{
	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			bputc(buf, *fmt);
			continue;
		}

		switch (*++fmt) {
		case 's':
			escape(buf, va_arg_(ap, char *));
			break;
		case 'S':
			bputs(buf, va_arg_(ap, char *));
			break;
		case 'i':
			bprint(buf, "%i", va_arg_(ap, int));
			break;
		case 'I':
			bprint(buf, "%li", va_arg_(ap, long));
			break;
		case 'u':
			bprint(buf, "%u", va_arg_(ap, unsigned));
			break;
		case 'U':
			bprint(buf, "%lu", va_arg_(ap, long unsigned));
			break;
		case 'f':
			bprint(buf, "%.1f", va_arg_(ap, double));
			break;
		case 'c':
			bputc(buf, va_arg_(ap, int));
			break;
		case '%':
			bputc(buf, '%');
			break;
		default:
			if (!extend || !extend(buf, *fmt, ap))
				error(500, "Unknown conversion specifier '%%%c'\n", *fmt);
		}
	}
}

/* html()-like functions all share a common dynamic buffer */
static struct dynbuf outbuf;

/*
 * We don't really need to free buffers when the program terminate, but
 * we want a clean valgrind report so that real memory leaks can easily
 * be spotted.
 */
static void free_output(void)
{
	free_dynbuf(&outbuf);
}

void reset_output(void)
{
	static bool initialized = false;

	if (!initialized) {
		atexit(free_output);
		initialized = true;
	}

	reset_dynbuf(&outbuf, NULL, 0);
}

void print_output(void)
{
	fwrite(outbuf.data, outbuf.pos, 1, stdout);
}

void css(const char *fmt, ...)
{
	va_list_ ap;
	va_start_(ap, fmt);
	print(&outbuf, bputs, NULL, fmt, ap);
	va_end_(ap);
}

void txt(const char *fmt, ...)
{
	va_list_ ap;
	va_start_(ap, fmt);
	print(&outbuf, bputs, NULL, fmt, ap);
	va_end_(ap);
}

static void escape_xml(struct dynbuf *buf, const char *str)
{
	for (; *str; str++) {
		switch (*str) {
		case '&':
			bputs(buf, "&amp;");
			break;
		case '<':
			bputs(buf, "&lt;");
			break;
		case '>':
			bputs(buf, "&gt;");
			break;
		case '\"':
			bputs(buf, "&quot;");
			break;
		case '\'':
			bputs(buf, "&#39;");
			break;
		default:
			bputc(buf, *str);
			break;
		}
	}
}

void html(const char *fmt, ...)
{
	va_list_ ap;
	va_start_(ap, fmt);
	print(&outbuf, escape_xml, NULL, fmt, ap);
	va_end_(ap);
}

void xml(const char *fmt, ...)
{
	va_list_ ap;
	va_start_(ap, fmt);
	print(&outbuf, escape_xml, NULL, fmt, ap);
	va_end_(ap);
}

void svg(const char *fmt, ...)
{
	va_list_ ap;
	va_start_(ap, fmt);
	print(&outbuf, escape_xml, NULL, fmt, ap);
	va_end_(ap);
}

/*
 * Not only we escape any special characters, but we also suround the
 * escaped string with quotes.
 */
static void escape_json(struct dynbuf *buf, const char *str)
{
	bputc(buf, '\"');

	for (; *str; str++) {
		switch (*str) {
		case '\b':
			bputs(buf, "\\b");
			break;
		case '\f':
			bputs(buf, "\\f");
			break;
		case '\n':
			bputs(buf, "\\n");
			break;
		case '\r':
			bputs(buf, "\\r");
			break;
		case '\t':
			bputs(buf, "\\t");
			break;
		case '\"':
			bputs(buf, "\\\"");
			break;
		case '\\':
			bputs(buf, "\\\\");
			break;
		default:
			bputc(buf, *str);
			break;
		}
	}

	bputc(buf, '\"');
}

static bool extend_json(struct dynbuf *buf, char c, va_list_ ap)
{
	/* Boolean */
	if (c == 'b') {
		if (va_arg_(ap, int))
			bputs(buf, "true");
		else
			bputs(buf, "false");
		return true;
	}

	/* Our JSON dates use RFC-3339 format */
	if (c == 'd') {
		char tmp[] = "yyyy-mm-ddThh:mm:ssZ";
		size_t ret;

		time_t t = va_arg_(ap, time_t);
		ret = strftime(tmp, sizeof(tmp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));

		bputc(buf, '\"');
		if (ret != sizeof(tmp) - 1)
			bputs(buf, "1970-00-00T00:00:00Z");
		else
			bputs(buf, tmp);
		bputc(buf, '\"');
		return true;
	}

	return false;
}

void json(const char *fmt, ...)
{
	va_list_ ap;
	va_start_(ap, fmt);
	print(&outbuf, escape_json, extend_json, fmt, ap);
	va_end_(ap);
}

/*
 * We only let through in URLs alpha-numeric characters as well as '-',
 * '_', '.'  and '~'.  Every other characters must be encoded.
 */
static bool safe_url_char(char c)
{
	if (isalnum(c))
		return 1;

	return strchr("-_.~", c) != NULL;
}

static char dectohex(char dec)
{
	switch (dec) {
	case 0: return '0';
	case 1: return '1';
	case 2: return '2';
	case 3: return '3';
	case 4: return '4';
	case 5: return '5';
	case 6: return '6';
	case 7: return '7';
	case 8: return '8';
	case 9: return '9';
	case 10: return 'A';
	case 11: return 'B';
	case 12: return 'C';
	case 13: return 'D';
	case 14: return 'E';
	case 15: return 'F';
	default: return '0';
	}
}

static void url_encode(struct dynbuf *buf, const char *str)
{
	for (; *str; str++) {
		if (safe_url_char(*str)) {
			bputc(buf, *str);
		} else {
			unsigned char c = *(unsigned char*)str;
			bputc(buf, '%');
			bputc(buf, dectohex(c / 16));
			bputc(buf, dectohex(c % 16));
		}
	}
}

static bool dynbuf_compare(struct dynbuf *buf, char *str)
{
	if (!buf->havedata)
		return !str;
	if (!str)
		return false;

	return strcmp(buf->data, str) == 0;
}

static void process_params(struct dynbuf *buf, va_list_ ap, char sep)
{
	struct dynbuf tmp;
	url_t tmpbuf;
	char *pname, *pdflt, *ptype;

	while ((pname = va_arg_(ap, char *))) {
		if (!*pname) {
			bputc(buf, '#');
			url_encode(buf, va_arg_(ap, char *));
			continue;
		}

		pdflt = va_arg_(ap, char *);
		ptype = va_arg_(ap, char *);

		reset_dynbuf(&tmp, tmpbuf, sizeof(tmpbuf));
		print(&tmp, bputs, NULL, ptype, ap);
		nul_terminate(&tmp);

		if (!dynbuf_compare(&tmp, pdflt)) {
			bputc(buf, sep);
			url_encode(buf, pname);
			if (tmp.havedata) {
				bputc(buf, '=');
				url_encode(buf, tmp.data);
			}

			if (sep == '?')
				sep = '&';
		}
	}
}

void URL_(url_t url, const char *prefix, ...)
{
	struct dynbuf buf;
	char sep = '?';
	va_list_ ap;

	reset_dynbuf(&buf, url, sizeof(url_t));

	/*
	 * If the prefix string don't have parameters, we wan to use '?'
	 * as our first separator for parameter.  Otherwise we use '&'.
	 * So we search for '?' while copying the prefix.
	 */
	for (; *prefix; prefix++) {
		if (*prefix == '?')
			sep = '&';
		bputc(&buf, *prefix);
	}

	va_start_(ap, prefix);
	process_params(&buf, ap, sep);
	va_end_(ap);

	nul_terminate(&buf);
}

char *URL_EXTRACT__(struct url *url, char *name, char *dflt)
{
	char *ret = NULL;
	unsigned i;

	for (i = 0; i < url->nargs; i++)
		if (strcmp(url->args[i].name, name) == 0)
			ret = url->args[i].val;

	if (!ret)
		ret = dflt;

	return ret;
}

void check_order_(char *order, ...)
{
	va_list ap;
	char *str;

	va_start(ap, order);
	while ((str = va_arg(ap, char *)))
		if (strcmp(str, "rank") != 0)
			break;
	va_end(ap);

	if (!str)
		error(400, "Invalid order \"%s\"", order);
}

static char *orderby(char *order)
{
	if (!order)
		return "";
	if (strcmp(order, "rank") == 0)
		return "rank";
	if (strcmp(order, "lastseen") == 0)
		return "lastseen DESC, rank";

	error(400, "Unkown order: \"%s\"\n", order);
	return NULL;
}

struct list init_simple_list(const char *qselect, const char *bindfmt, ...)
{
	struct list list = { 0 };
	va_list ap;

	va_start(ap, bindfmt);
	list.res = foreach_init_(qselect, bindfmt, ap);
	va_end(ap);

	return list;
}

struct list init_list(
	const char *qselect, const char *qcount,
	unsigned plen, unsigned pnum, char *order,
	const char *bindfmt, ...)
{
	struct list list;
	char fmt[512], qry[512];
	va_list ap;
	int ret;

	assert(qselect != NULL);
	assert(qcount != NULL);

	ret = snprintf(
		fmt, sizeof(fmt), "%s LIMIT %u OFFSET %u",
		qselect, plen, (pnum - 1) * plen
	);
	if (ret >= sizeof(fmt))
		error(500, "init_list: qselect too long");

	ret = snprintf(qry, sizeof(qry), fmt, orderby(order));
	if (ret >= sizeof(qry))
		error(500, "init_list: qselect too long");

	va_start(ap, bindfmt);
	list.res = foreach_init_(qry, bindfmt, ap);
	va_end(ap);

	if (qcount) {
		va_start(ap, bindfmt);
		list.nrow = count_rows__(qcount, bindfmt, ap);
		va_end(ap);
	}

	list.pnum = pnum;
	list.plen = plen;
	list.order = order;

	return list;
}
