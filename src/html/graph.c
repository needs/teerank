#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "player.h"
#include "html.h"

/*
 * Graph works with numbers, however numbers can be signed or
 * unsigned.  For instance elo points are signed but rank are
 * unsigned.  Having separate cases for both signed un unsigned
 * numbers would make the code highly redundant.  Instead we just
 * choose to use "long" as the only accepted number, and data need to
 * be converted to fit in a long.
 *
 * This may have some issue because domain(int) <= domain(long), hence
 * LONG_MAX < UINT_MAX, so some overflow checking must be done when
 * dealing with unsigned numbers, sadly.
 *
 * I wich there was some nice way to handle signed and unsigned
 * numbers.  One way could be to use pointers to functions but it
 * would not be enough because printf() format string as to be adapted
 * too.  So just forcing "long" for data seems to be the best compromise...
 */

typedef long (*to_long_t)(const void *data);

static long elo_to_long(const void *data)
{
	return (long)*(int*)data;
}

static long rank_to_long(const void *data)
{
	unsigned rank = *(unsigned*)data;

	if (rank > LONG_MAX)
		return LONG_MAX;

	return (long)rank;
}

static long find_data(
	struct historic *hist, int (*cmp)(long, long), to_long_t to_long)
{
	struct record *rec;
	long ret;

	assert(hist != NULL);
	assert(cmp != NULL);
	assert(to_long != NULL);
	assert(hist->nrecords > 0);
	assert(hist->first != NULL);

	ret = to_long(record_data(hist, hist->first));
	for (rec = hist->first->next; rec; rec = rec->next) {
		long data = to_long(record_data(hist, rec));

		if (cmp(data, ret))
			ret = data;
	}

	return ret;
}

/* We want at least 3 axes on the graph */
#define MINIMUM_NUMBER_OF_AXES 3

/*
 * Given a gap (plus a multiplier) and a range of values, return true if
 * at least MINIMUM_NUMBER_OF_AXES axes can be displayed.
 */
static int does_fit(unsigned range, unsigned gap, unsigned factor)
{
	return MINIMUM_NUMBER_OF_AXES * gap * factor < range;
}

#define DEFAULT_GAP 100

static unsigned best_axes_gap(unsigned range)
{
	static const unsigned gaps[] = { 1, 2, 5 };
	unsigned i = 0, factor = 1;

	while (does_fit(range, gaps[i], factor)) {
		i = (i + 1) % 3;
		if (i == 0)
			factor *= 10;
	}

	/* Return previous gap, or the default gap if any */
	if (i == 0 && factor == 1)
		return DEFAULT_GAP;
	else if (i == 0)
		return gaps[2] * (factor / 10);
	else
		return gaps[i - 1] * factor;
}

/*
 * When called we now that *at least* MINIMUM_NUMBER_OF_AXES fits in the
 * given range (max - min).  However even more axes can be displayed.  This
 * function get the maximum number of axes that can be displayed in the
 * given range.
 */
static unsigned number_of_axes(int min, int max, unsigned gap)
{
	unsigned n = MINIMUM_NUMBER_OF_AXES;
	int count = min - (min % gap) + 3 * gap;

	while (count < max) {
		n++;
		count += gap;
	}

	return n - 1 < MINIMUM_NUMBER_OF_AXES ? MINIMUM_NUMBER_OF_AXES : n - 1;
}

/*
 * Carry the context necessary to draw the graph.
 */
struct graph {
	struct historic *hist;
	int is_empty;

	to_long_t to_long;

	/*
	 * The minimum (and maximum) value  that can be plotted in the graph.
	 *
	 * These values are not the same than the minimum (and maximum) values
	 * the will be plotted in the graph.  They are used to compute the
	 * offset of point to be plotted.
	 */
	long min, max;

	/* Number of axes and gap between each axes. */
	unsigned naxes, gap;
};

static int min_cmp(long a, long b)
{
	return a < b;
}

static int max_cmp(long a, long b)
{
	return a > b;
}

static struct graph init_graph(
	struct historic *hist, to_long_t to_long)
{
	static const struct graph GRAPH_ZERO;
	struct graph graph = GRAPH_ZERO;
	long min, max;

	graph.hist = hist;
	graph.to_long = to_long;

	if (hist->nrecords == 0) {
		graph.is_empty = 1;
		return graph;
	}

	min = find_data(hist, min_cmp, to_long);
	max = find_data(hist, max_cmp, to_long);

	graph.gap = best_axes_gap(max - min);
	graph.naxes = number_of_axes(min, max, graph.gap);

	/*
	 * We round down the minimum and maximum values so that axes are
	 * evenly spaced between themself and borders.
	 */
	graph.min = min - (min % graph.gap);
	graph.max = graph.min + (graph.naxes + 1) * graph.gap;

	return graph;
}

static long axe_offset(struct graph *graph, unsigned i)
{
	return graph->min + (i + 1) * graph->gap;
}

static void print_axes(struct graph *graph)
{
	unsigned i;

	svg("<!-- Axes -->");
	svg("<g>");
	for (i = 0; i < graph->naxes; i++) {
		const float y = 100.0 - (i + 1) * (100.0 / ((graph->naxes + 1)));
		const long label = axe_offset(graph, i);

		if (i)
			svg("");

		/* Line */
		svg("<line x1=\"0\" y1=\"%.1f%%\" x2=\"100%%\" y2=\"%.1f%%\" stroke=\"#bbb\" stroke-dasharray=\"3, 3\"/>", y , y);

		/* Left and right labels */
		svg("<text x=\"10\" y=\"%.1f%%\" style=\"fill: #777; font-size: 0.9em; dominant-baseline: middle;\">%ld</text>", y, label);
		svg("<text x=\"100%%\" y=\"%.1f%%\" style=\"fill: #777; font-size: 0.9em; dominant-baseline: middle; text-anchor: end;\" transform=\"translate(-10, 0)\">%ld</text>", y, label);
	}
	svg("</g>");
}

struct point {
	float x, y;
};

/*
 * Given an entry compute the coordinates of the point on the graph.
 */
struct point init_point(struct graph *graph, struct record *record)
{
	struct point p;
	long data;

	assert(graph != NULL);
	assert(record != NULL);

	data = graph->to_long(record_data(graph->hist, record));

	if (graph->hist->nrecords - 1 == 0)
		p.x = 0;
	else
		p.x = (float)((record - graph->hist->records)) / (graph->hist->nrecords - 1) * 100.0;

	if (graph->max - graph->min == 0)
		p.y = 0;
	else
		p.y = 100.0 - ((float)(data - graph->min) / (graph->max - graph->min) * 100.0);

	return p;
}

static void print_line(struct graph *graph, struct record *a, struct record *b)
{
	struct point pa, pb;

	assert(a != NULL);
	assert(b != NULL);

	pa = init_point(graph, a);
	pb = init_point(graph, b);

	svg("<line x1=\"%.1f%%\" y1=\"%.1f%%\" x2=\"%.1f%%\" y2=\"%.1f%%\" style=\"fill: none; stroke:#970; stroke-width: 3px;\"/>", pa.x, pa.y, pb.x, pb.y);
}

static void print_lines(struct graph *graph)
{
	struct record *rec;

	assert(graph != NULL);
	assert(!graph->is_empty);

	svg("<!-- Path -->");
	svg("<g>");

	for (rec = graph->hist->first->next; rec; rec = rec->next)
		print_line(graph, rec->prev, rec);

	svg("</g>");
}

static const char *point_label_pos(struct graph *graph, struct record *record, long data)
{
	const char *top_left = "text-anchor: end;\" transform=\"translate(-10, -11)";
	const char *top_right = "text-anchor: start;\" transform=\"translate(10, -11)";
	const char *bottom_left = "text-anchor: end;\" transform=\"translate(-10, 18)";
	const char *bottom_right = "text-anchor: start;\" transform=\"translate(10, 18)";

	/*
	 * Label are by default bottom right.
	 *
	 * If the curve is going down, the the label should placed on
	 * top of it.  If it's going, up, label should be below it.
	 *
	 * Eventually if there is no next point, label should be on
	 * the left, and we apply again the same reasoning exposed
	 * above, except we look at the previous point.
	 */
	if (record->next) {
		long next_data = graph->to_long(record_data(graph->hist, record->next));

		if (next_data > data)
			return bottom_right;
		else
			return top_right;
	} else if (record->prev) {
		long prev_data = graph->to_long(record_data(graph->hist, record->prev));

		if (prev_data > data)
			return bottom_left;
		else
			return top_left;
	} else {
		return bottom_right;
	}
}

static void print_point(struct graph *graph, struct record *record)
{
	struct point p;
	unsigned i;
	long data;
	const char *label_pos;
	float zone_width;

	assert(graph != NULL);
	assert(record != NULL);

	p = init_point(graph, record);
	i = record - graph->hist->records;
	data = graph->to_long(record_data(graph->hist, record));

	label_pos = point_label_pos(graph, record, data);

	/*
	 * Too much points in a graph reduce readability, above 24
	 * points, don't draw them anymore.
	 */
	if (graph->hist->nrecords <= 24)
		svg("<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\" style=\"fill: #970;\"/>", p.x, p.y);

	/* Hover */
	if (graph->hist->nrecords == 1)
		zone_width = 100.0;
	else
		zone_width = 1.0 / (graph->hist->nrecords - 1) * 100.0;

	svg("<g style=\"visibility: hidden\">");
	svg("<line x1=\"%.1f%%\" y1=\"0%%\" x2=\"%.1f%%\" y2=\"100%%\" style=\"stroke: #bbb;\"/>", p.x, p.x);
	svg("<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\" style=\"fill: #725800;\"/>", p.x, p.y);
	svg("<text x=\"%.1f%%\" y=\"%.1f%%\" style=\"font-size: 0.9em; %s\">%ld</text>", p.x, p.y, label_pos, data);
	svg("<set attributeName=\"visibility\" to=\"visible\" begin=\"zone%u.mouseover\" end=\"zone%u.mouseout\"/>", i, i);
	svg("</g>");
	svg("<rect id=\"zone%u\" x=\"%.1f%%\" y=\"0%%\" width=\"%.1f%%\" height=\"100%%\" style=\"fill: black; fill-opacity: 0;\"/>", i, p.x - zone_width / 2.0, zone_width);
}

static void print_points(struct graph *graph)
{
	struct record *rec;

	assert(graph != NULL);

	svg("<!-- Points -->");
	svg("<g>");

	for (rec = graph->hist->first; rec; rec = rec->next) {
		if (rec->prev)
			svg("");
		print_point(graph, rec);
	}

	svg("</g>");
}

static void print_notice_empty(struct graph *graph)
{
	assert(graph != NULL);

	svg("<text x=\"50%%\" y=\"50%%\" style=\"font-size: 0.9em; text-anchor: middle;\">No data available</text>");
}

static void print_graph(
	struct historic *hist, to_long_t to_long)
{
	struct graph graph;

	assert(hist != NULL);

	graph = init_graph(hist, to_long);

	svg("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	svg("<svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\" style=\"font-family: Verdana,Arial,Helvetica,sans-serif;\">");

	if (graph.is_empty) {
		print_notice_empty(&graph);
	} else {
		print_axes(&graph);
		svg("");
		print_lines(&graph);
		svg("");
		print_points(&graph);
	}

	svg("</svg>");
}

int main(int argc, char **argv)
{
	struct player player;
	char *name, *dataset;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <player_name> [elo|rank]\n", argv[0]);
		return EXIT_FAILURE;
	}

	name = argv[1];

	if (!is_valid_hexname(name)) {
		fprintf(stderr, "\"%s\" is not a valid player name\n", name);
		return EXIT_FAILURE;
	}

	init_player(&player);
	if (!read_player(&player, name))
		return EXIT_FAILURE;

	dataset = argv[2];
	if (strcmp(dataset, "elo") == 0) {
		print_graph(&player.elo_historic, elo_to_long);
	} else if (strcmp(dataset, "rank") == 0) {
		print_graph(&player.daily_rank, rank_to_long);
	} else {
		fprintf(stderr, "\"%s\" is not a valid dataset, expected \"elo\" or \"rank\"\n", dataset);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
