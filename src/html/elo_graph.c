#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "player.h"

static struct record *min_record(struct historic *hist, int (*cmp)(const void *a, const void *b))
{
	struct record *min = NULL;
	unsigned i;

	assert(hist != NULL);
	assert(cmp != NULL);

	for (i = 0; i < hist->nrecords; i++) {
		struct record *record = &hist->records[i];
		const void *data = record_data(hist, record);

		if (!min || cmp(min, data) == -1)
			min = record;
	}

	return min;
}

static struct record *max_record(struct historic *hist, int (*cmp)(const void *a, const void *b))
{
	struct record *max = NULL;
	unsigned i;

	assert(hist != NULL);
	assert(cmp != NULL);

	for (i = 0; i < hist->nrecords; i++) {
		struct record *record = &hist->records[i];
		const void *data = record_data(hist, record);

		if (!max || cmp(max, data) == 1)
			max = record;
	}

	return max;
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

static int cmp_elo(const void *pa, const void *pb)
{
	const int a = *(int*)pa;
	const int b = *(int*)pb;

	return a - b;
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

	min = min_record(hist, cmp_elo);
	max = max_record(hist, cmp_elo);

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

	printf("\t<!-- Axes -->\n");
	for (i = 0; i < graph->naxes; i++) {
		const float y = 100.0 - (i + 1) * (100.0 / ((graph->naxes + 1)));
		const int label = axe_offset(graph, i);

		/* Line */
		printf("\n\t<line x1=\"0\" y1=\"%.1f%%\" x2=\"100%%\" y2=\"%.1f%%\" stroke=\"#bbb\" stroke-dasharray=\"3, 3\"/>\n", y , y);

		/* Left and right labels */
		printf("\t<text x=\"10\" y=\"%.1f%%\" style=\"fill: #777; font-size: 0.9em; dominant-baseline: middle;\">%d</text>\n", y, label);
		printf("\t<text x=\"100%%\" y=\"%.1f%%\" style=\"fill: #777; font-size: 0.9em; dominant-baseline: middle; text-anchor: end;\" transform=\"translate(-10, 0)\">%d</text>\n", y, label);
	}
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

	printf("\t\t<line x1=\"%.1f%%\" y1=\"%.1f%%\" x2=\"%.1f%%\" y2=\"%.1f%%\" style=\"fill: none; stroke:#970; stroke-width: 3px;\"/>\n", pa.x, pa.y, pb.x, pb.y);
}

static void print_lines(struct graph *graph)
{
	unsigned i;

	assert(graph != NULL);

	printf("\n\t<!-- Path -->\n");
	printf("\t<g>\n");

	for (i = 1; i < graph->hist->nrecords; i++) {
		struct record *prev, *current;

		prev = &graph->hist->records[i - 1];
		current = &graph->hist->records[i];

		print_line(graph, prev, current);
	}

	printf("\t</g>\n");
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

	printf("\t\t<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\" style=\"fill: #970;\"/>\n", p.x, p.y);

	/* Hover */
	printf("\t\t<g style=\"visibility: hidden\">\n");
	printf("\t\t\t<line x1=\"%.1f%%\" y1=\"0%%\" x2=\"%.1f%%\" y2=\"100%%\" style=\"stroke: #bbb;\"/>\n", p.x, p.x);
	printf("\t\t\t<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\" style=\"fill: #725800;\"/>\n", p.x, p.y);
	printf("\t\t\t<text x=\"%.1f%%\" y=\"%.1f%%\" style=\"font-size: 0.9em; text-anchor: start;\" transform=\"translate(10, 18)\">%d</text>\n", p.x, p.y, elo);
	printf("\t\t\t<set attributeName=\"visibility\" to=\"visible\" begin=\"zone%u.mouseover\" end=\"zone%u.mouseout\"/>\n", i, i);
	printf("\t\t</g>\n");
	printf("\t\t<rect id=\"zone%u\" x=\"%.1f%%\" y=\"0%%\" width=\"10%%\" height=\"100%%\" style=\"fill: black; fill-opacity: 0;\"/>\n", i, p.x - 5);
}

static void print_points(struct graph *graph)
{
	unsigned i;

	assert(graph != NULL);

	printf("\n\t<!-- Points -->\n");
	printf("\t<g>\n");

	for (i = 0; i < graph->hist->nrecords; i++)
		print_point(graph, &graph->hist->records[i]);

	printf("\t</g>\n");
}

static void print_notice_empty(struct graph *graph)
{
	assert(graph != NULL);

	printf("\t<text x=\"50%%\" y=\"50%%\" style=\"font-size: 0.9em; text-anchor: middle;\">No data available</text>\n");
}

static void print_graph(struct player *player)
{
	struct graph graph;

	assert(player != NULL);

	graph = init_graph(&player->elo_historic);

	printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	printf("<svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\">\n");

	if (graph.is_empty) {
		print_notice_empty(&graph);
	} else {
		print_axes(&graph);
		print_lines(&graph);
		print_points(&graph);
	}

	printf("</svg>\n");
}

int main(int argc, char **argv)
{
	struct player player = PLAYER_ZERO;
	char *name;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	name = argv[1];

	if (!is_valid_hexname(name)) {
		fprintf(stderr, "\"%s\" is not a valid player name", name);
		return EXIT_FAILURE;
	}

	if (!read_player(&player, name))
		return EXIT_FAILURE;

	print_graph(&player);

	return EXIT_SUCCESS;
}
