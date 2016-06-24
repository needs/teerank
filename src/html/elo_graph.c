#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "player.h"
#include "html.h"

typedef int (*record_cmp_t )(const void *a, const void *b);

static struct record *find_record(
	struct historic *hist, record_cmp_t cmp)
{
	struct record *ret = NULL;
	unsigned i;

	assert(hist != NULL);
	assert(cmp != NULL);

	for (i = 0; i < hist->nrecords; i++) {
		struct record *rec = &hist->records[i];
		const void *data = record_data(hist, rec);

		if (!ret || cmp(ret, data))
			ret = rec;
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
		factor *= 10;
		i = (i + 1) % 3;
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
	int is_empty;

	/*
	 * The minimum (and maximum) value  that can be plotted in the graph.
	 *
	 * These values are not the same than the minimum (and maximum) values
	 * the will be plotted in the graph.  They are used to compute the
	 * offset of point to be plotted.
	 */
	int min, max;

	/* Number of axes and gap between each axes. */
	unsigned naxes, gap;

	struct historic *hist;
};

static int min_elo_cmp(const void *a, const void *b)
{
	return *(int*)a < *(int*)b;
}

static int max_elo_cmp(const void *a, const void *b)
{
	return *(int*)a > *(int*)b;
}

static int get_elo(struct historic *hist, struct record *record)
{
	return *(int*)record_data(hist, record);
}

static struct graph init_graph(struct historic *hist)
{
	struct graph graph;
	struct record *min, *max;
	int min_elo, max_elo;

	min = find_record(hist, min_elo_cmp);
	max = find_record(hist, max_elo_cmp);

	graph.is_empty = (min == NULL || max == NULL);
	if (graph.is_empty)
		return graph;

	min_elo = get_elo(hist, min);
	max_elo = get_elo(hist, max);

	graph.gap = best_axes_gap(max_elo - min_elo);
	graph.naxes = number_of_axes(min_elo, max_elo, graph.gap);

	/*
	 * We round down the minimum and maximum values so that axes are
	 * evenly spaced between themself and borders.
	 */
	graph.min = min_elo - (min_elo % graph.gap);
	graph.max = max_elo - (max_elo % graph.gap) + graph.gap;

	graph.hist = hist;

	return graph;
}

static int axe_offset(struct graph *graph, unsigned i)
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
		const int label = axe_offset(graph, i);

		if (i)
			svg("");

		/* Line */
		svg("<line x1=\"0\" y1=\"%.1f%%\" x2=\"100%%\" y2=\"%.1f%%\" stroke=\"#bbb\" stroke-dasharray=\"3, 3\"/>", y , y);

		/* Left and right labels */
		svg("<text x=\"10\" y=\"%.1f%%\" style=\"fill: #777; font-size: 0.9em; dominant-baseline: middle;\">%d</text>", y, label);
		svg("<text x=\"100%%\" y=\"%.1f%%\" style=\"fill: #777; font-size: 0.9em; dominant-baseline: middle; text-anchor: end;\" transform=\"translate(-10, 0)\">%d</text>", y, label);
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
	int elo;

	assert(graph != NULL);
	assert(record != NULL);

	elo = get_elo(graph->hist, record);

	if (graph->hist->nrecords - 1 == 0)
		p.x = 0;
	else
		p.x = (float)((record - graph->hist->records)) / (graph->hist->nrecords - 1) * 100.0;

	if (graph->max - graph->min == 0)
		p.y = 0;
	else
		p.y = 100.0 - ((float)(elo - graph->min) / (graph->max - graph->min) * 100.0);

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
	unsigned i;

	assert(graph != NULL);

	svg("<!-- Path -->");
	svg("<g>");

	for (i = 1; i < graph->hist->nrecords; i++) {
		struct record *prev, *current;

		prev = &graph->hist->records[i - 1];
		current = &graph->hist->records[i];

		print_line(graph, prev, current);
	}

	svg("</g>");
}

static void print_point(struct graph *graph, struct record *record)
{
	struct point p;
	unsigned i;
	int elo;

	assert(graph != NULL);
	assert(record != NULL);

	p = init_point(graph, record);
	i = record - graph->hist->records;
	elo = get_elo(graph->hist, record);

	svg("<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\" style=\"fill: #970;\"/>", p.x, p.y);

	/* Hover */
	svg("<g style=\"visibility: hidden\">");
	svg("<line x1=\"%.1f%%\" y1=\"0%%\" x2=\"%.1f%%\" y2=\"100%%\" style=\"stroke: #bbb;\"/>", p.x, p.x);
	svg("<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\" style=\"fill: #725800;\"/>", p.x, p.y);
	svg("<text x=\"%.1f%%\" y=\"%.1f%%\" style=\"font-size: 0.9em; text-anchor: start;\" transform=\"translate(10, 18)\">%d</text>", p.x, p.y, elo);
	svg("<set attributeName=\"visibility\" to=\"visible\" begin=\"zone%u.mouseover\" end=\"zone%u.mouseout\"/>", i, i);
	svg("</g>");
	svg("<rect id=\"zone%u\" x=\"%.1f%%\" y=\"0%%\" width=\"10%%\" height=\"100%%\" style=\"fill: black; fill-opacity: 0;\"/>", i, p.x - 5);
}

static void print_points(struct graph *graph)
{
	unsigned i;

	assert(graph != NULL);

	svg("<!-- Points -->");
	svg("<g>");

	for (i = 0; i < graph->hist->nrecords; i++) {
		if (i)
			svg("");
		print_point(graph, &graph->hist->records[i]);
	}

	svg("</g>");
}

static void print_notice_empty(struct graph *graph)
{
	assert(graph != NULL);

	svg("<text x=\"50%%\" y=\"50%%\" style=\"font-size: 0.9em; text-anchor: middle;\">No data available</text>");
}

static void print_graph(struct historic *hist)
{
	struct graph graph;

	assert(hist != NULL);

	graph = init_graph(hist);

	svg("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	svg("<svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\">");

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
	char *name;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <player_name>\n", argv[0]);
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

	print_graph(&player.elo_historic);

	return EXIT_SUCCESS;
}
