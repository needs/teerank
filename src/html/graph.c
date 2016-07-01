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
	return (long)((const struct player_record*)(data))->elo;
}

static long rank_to_long(const void *data)
{
	unsigned rank = ((const struct player_record*)(data))->rank;

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
#define MIN_NAXES 3

/*
 * Given a gap (plus a multiplier) and a range of values, return true if
 * at least MIN_NAXES axes can be displayed.
 */
static int does_fit(unsigned range, unsigned gap, unsigned factor)
{
	return MIN_NAXES * gap * factor < range;
}

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
		return 1;
	else if (i == 0)
		return gaps[2] * (factor / 10);
	else
		return gaps[i - 1] * factor;
}

/*
 * When called we now that *at least* MIN_NAXES fits in the
 * given range (max - min).  However even more axes can be displayed.  This
 * function get the maximum number of axes that can be displayed in the
 * given range.
 */
static unsigned number_of_axes(int min, int max, unsigned gap)
{
	unsigned n = MIN_NAXES;
	int count = min - (min % gap) + 3 * gap;

	while (count < max) {
		n++;
		count += gap;
	}

	return n - 1 < MIN_NAXES ? MIN_NAXES : n - 1;
}

struct curve {
	to_long_t to_long;
	int reversed;

	/*
	 * The minimum (and maximum) value that can be plotted in the graph.
	 *
	 * These values are not the same than the minimum (and maximum) values
	 * that *will* be plotted in the graph.  They are used to compute the
	 * offset of point to be plotted.
	 */
	long ymin, ymax;

	unsigned padding_top, padding_bottom;
	unsigned gap;
};

/* Only two curves are handled because placing axes is not trivial */
#define MAX_CURVES 2

struct graph {
	struct historic *hist;

	/*
	 * Number of axes is shared between curves so we don't mix up
	 * graph of each curve.
	 */
	unsigned naxes;

	unsigned ncurves;
	struct curve curves[MAX_CURVES];
};

static int min_cmp(long a, long b)
{
	return a < b;
}

static int max_cmp(long a, long b)
{
	return a > b;
}

static struct graph init_graph(struct historic *hist)
{
	static const struct graph GRAPH_ZERO;
	struct graph graph = GRAPH_ZERO;

	graph.hist = hist;

	return graph;
}

/*
 * Scale a curve to the new number of axes
 */
static void scale_curve(struct curve *curve, unsigned old_naxes, unsigned new_naxes)
{
	unsigned half, remainder;
	unsigned top, bottom;

	assert(new_naxes > old_naxes);

	half = (new_naxes - old_naxes) / 2;
	remainder = (new_naxes - old_naxes) % 2;

	if (curve->padding_top == curve->padding_bottom) {
		top = half + remainder;
		bottom = half;
	} else {
		top = half;
		bottom = half + remainder;
	}

	curve->padding_top += top;
	curve->padding_bottom += bottom;
	curve->ymin -= bottom * curve->gap;
	curve->ymax += top * curve->gap;
}

static void add_curve(
	struct graph *graph, to_long_t to_long, int reversed)
{
	struct curve *curve;
	long ymin, ymax;
	unsigned naxes;

	assert(graph->ncurves < MAX_CURVES);
	curve = &graph->curves[graph->ncurves];
	graph->ncurves++;

	curve->to_long = to_long;
	curve->reversed = reversed;

	if (graph->hist->nrecords == 0)
		return;

	ymin = find_data(graph->hist, min_cmp, to_long);
	ymax = find_data(graph->hist, max_cmp, to_long);

	curve->gap = best_axes_gap(ymax - ymin);
	naxes = number_of_axes(ymin, ymax, curve->gap);

	curve->padding_top = 0;
	curve->padding_bottom = 0;

	if (naxes > graph->naxes) {
		unsigned i;
		for (i = 0; i < graph->ncurves - 1; i++)
			scale_curve(&graph->curves[i], graph->naxes, naxes);
		graph->naxes = naxes;
	}

	/*
	 * We round down the minimum and maximum values so that axes are
	 * evenly spaced between themself and borders.
	 */
	curve->ymin = ymin - (ymin % curve->gap);
	curve->ymax = curve->ymin + (graph->naxes + 1) * curve->gap;
}

static long axe_data(struct curve *curve, unsigned i)
{
	return curve->ymin + (i + 1) * curve->gap;
}

static float pad(float value, float left, float right)
{
	float scale = (100.0 - left - right) / 100.0;
	return left + value * scale;
}

static float pad_x(struct graph *graph, float x)
{
	const float PADDING = 2.0;
	const float LABEL_SIZE = 4.0;

	const unsigned nleft = graph->ncurves / 2.0 + graph->ncurves % 2;
	const unsigned nright = graph->ncurves / 2.0;

	const float padding_left = PADDING + nleft * LABEL_SIZE;
	const float padding_right = PADDING + nright * LABEL_SIZE;

	return pad(x, padding_left, padding_right);
}

static float pad_y(struct graph *graph, float y)
{
	const float PADDING = 1.0;
	return pad(y, PADDING, PADDING);
}

static float percentage(float range, float value, int reversed)
{
	float ret;

	if (range == 0.0)
		return 0.0;

	ret = value / range * 100.0;
	if (!reversed)
		ret = 100.0 - ret;

	if (ret < 0.0)
		return 0.0;
	else if (ret > 100.0)
		return 100.0;

	return ret;
}

static float x_coord(
	struct graph *graph, struct curve *curve, struct record *record)
{
	float range, value;

	range = graph->hist->nrecords - 1;
	value = record - graph->hist->records;

	return pad_x(graph, percentage(range, value, 1));
}

static float y_coord(struct graph *graph, struct curve *curve, long data)
{
	float range, value;

	range = curve->ymax - curve->ymin;
	value = data - curve->ymin;

	return pad_y(graph, percentage(range, value, curve->reversed));
}

static void print_axes(struct graph *graph)
{
	unsigned i, j;
	float x_start, x_end;

	x_start = pad_x(graph, 0) - 1.0;
	x_end = pad_x(graph, 100) + 1.0;

	svg("<!-- Axes -->");
	svg("<style>");
	css(".axe {");
	css("stroke: #bbb;");
	css("stroke-dasharray: 3, 3;");
	css("}");
	css(".axe_label {");
	css("fill: #777;");
	css("font-size: 0.9em;");
	css("dominant-baseline: middle;");
	css("}");
	css(".axe_label.left {");
	css("transform: translate(10, 0);");
	css("text-anchor: start;");
	css("}");
	css(".axe_label.right {");
	css("transform: translate(-10, 0);");
	css("text-anchor: end;");
	css("}");
	svg("</style>");

	svg("<g>");

	/* Axes */
	for (i = 0; i < graph->naxes; i++) {
		struct curve *curve = &graph->curves[0];
		long data = axe_data(curve, i);
		const float y = y_coord(graph, curve, data);

		/* Line */
		svg("<line class=\"axe\" x1=\"%.1f%%\" y1=\"%.1f%%\" x2=\"%.1f%%\" y2=\"%.1f%%\"/>",
		    x_start, y , x_end, y);
	}
	svg("");

	/* Labels */
	for (i = 0; i < graph->ncurves; i++) {
		struct curve *curve = &graph->curves[i];

		for (j = 0; j < graph->naxes; j++) {
			long data;
			float y;
			float x;
			const char *class;

			data = axe_data(curve, j);
			y = y_coord(graph, curve, data);

			if (i % 2 == 0) {
				class = "left";
				x = 0.0;
			} else if (i % 2 == 1) {
				class = "right";
				x = 100.0;
			}

			svg("<text class=\"axe_label %s\" x=\"%.1f%%\" y=\"%.1f%%\">%ld</text>",
			    class, x, y, data);
		}
	}
	svg("</g>");
}

struct point {
	float x, y;
};

/*
 * Given an entry compute the coordinates of the point on the curve.
 */
struct point init_point(
	struct graph *graph, struct curve *curve, struct record *record)
{
	struct point p;
	long data;

	assert(curve != NULL);
	assert(record != NULL);

	data = curve->to_long(record_data(graph->hist, record));

	p.x = x_coord(graph, curve, record);
	p.y = y_coord(graph, curve, data);

	return p;
}

static void print_path(struct graph *graph, struct curve *curve)
{
	struct record *rec;

	assert(curve != NULL);

	if (graph->hist->nrecords < 2)
		return;

	svg("<!-- Path -->");
	svg("<svg viewBox=\"0 0 100 100\" preserveAspectRatio=\"none\">");

	/* No need to create a class for a single element */
	svg("<path style=\"fill: none; stroke: #970; stroke-width: 3px;\" d=\"");
	for (rec = graph->hist->first; rec; rec = rec->next) {
		struct point p;
		char c;

		if (rec == graph->hist->first)
			c = 'M';
		else
			c = 'L';

		p = init_point(graph, curve, rec);
		svg("%c %.1f %.1f", c, p.x, p.y);

	}
	svg("\" vector-effect=\"non-scaling-stroke\"/>");
	svg("</svg>");
}

static const char *point_label_pos(
	struct graph *graph, struct curve *curve,
	struct record *record, long data, struct point p)
{
	const char *top_left = "top_left";
	const char *top_right = "top_right";
	const char *bottom_left = "bottom_left";
	const char *bottom_right = "bottom_right";

	const float X_MARGIN = 8.0;
	const float Y_MARGIN = 8.0;

	if (curve->reversed) {
		const char *tmp;

		tmp = top_left;
		top_left = bottom_left;
		bottom_left = tmp;

		tmp = top_right;
		top_right = bottom_right;
		bottom_right = tmp;

		/* Hacky */
		p.y = 100.0 - p.y;
	}

	/*
	 * Label are by default bottom right.
	 *
	 * If the curve is going down, the the label should placed on
	 * top of it.  If it's going, up, label should be below it.
	 *
	 * Eventually if there is no next point, label should be on
	 * the left, and we apply again the same reasoning exposed
	 * above, except we look at the previous point.
	 *
	 * When label are too close to a border, then its position is
	 * enforced to make it sure it is inside svg rendering area.
	 */
	if (p.x > 100.0 - X_MARGIN) {
		long prev_data;

		if (p.y > 100.0 - Y_MARGIN)
			return top_left;
		else if (p.y < Y_MARGIN)
			return bottom_left;

		if (!record->prev)
			return bottom_left;

		prev_data = curve->to_long(record_data(graph->hist, record->prev));

		if (prev_data > data)
			return bottom_left;
		else
			return top_left;
	} else {
		long next_data;

		if (p.y > 100.0 - Y_MARGIN)
			return top_right;
		else if (p.y < Y_MARGIN)
			return bottom_right;

		if (!record->next)
			return bottom_right;

		next_data = curve->to_long(record_data(graph->hist, record->next));

		if (next_data > data)
			return bottom_right;
		else
			return top_right;
	}
}

static void print_zone(struct graph *graph, struct record *rec, struct point p)
{
	float gap, zone_start, zone_width;

	if (graph->hist->nrecords == 1)
		gap = 100.0;
	else {
		const float unpadded_gap = 100.0 / (graph->hist->nrecords - 1);
		gap = pad_x(graph, unpadded_gap) - pad_x(graph, 0.0);
	}

	if (rec == graph->hist->first) {
		zone_start = 0.0;
		zone_width = pad_x(graph, 0.0) + gap / 2.0;
	} else if (rec == graph->hist->last) {
		zone_start = p.x - gap / 2.0;
		zone_width = pad_x(graph, 100.0) + gap / 2.0;
	} else {
		zone_start = p.x - gap / 2.0;
		zone_width = gap;
	}

	svg("<rect class=\"zone\" x=\"%.1f%%\" y=\"0%%\" width=\"%.1f%%\" height=\"100%%\"/>",
	    zone_start, zone_width);
}

static void print_label(struct point p, long data, const char *label_pos)
{
	svg("<circle cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\"/>",
	    p.x, p.y);
	svg("<text class=\"%s\" x=\"%.1f%%\" y=\"%.1f%%\">%ld</text>",
	    label_pos, p.x, p.y, data);
}

static void print_labels(struct graph *graph)
{
	struct record *rec;
	const char *label_pos;
	unsigned i;

	for (rec = graph->hist->first; rec; rec = rec->next) {
		struct point p;
		long data;

		p = init_point(graph, &graph->curves[0], rec);

		print_zone(graph, rec, p);

		svg("<g class=\"labels\">");
		svg("<line x1=\"%.1f%%\" y1=\"%.1f%%\" x2=\"%.1f%%\" y2=\"%.1f%%\"/>",
		    p.x, pad_y(graph, 0.0), p.x, pad_y(graph, 100.0));

		for (i = 0; i < graph->ncurves; i++) {
			struct curve *curve = &graph->curves[i];

			p = init_point(graph, curve, rec);
			data = curve->to_long(record_data(graph->hist, rec));
			label_pos = point_label_pos(graph, curve, rec, data, p);
			print_label(p, data, label_pos);
		}

		svg("</g>");
	}

}

static void print_point(
	struct graph *graph, struct curve *curve, struct record *record)
{
	struct point p;

	assert(curve != NULL);
	assert(record != NULL);

	p = init_point(graph, curve, record);

	/*
	 * Too much points in a curve reduce readability, above 24
	 * points, don't draw them anymore.
	 */
	if (record == graph->hist->last || graph->hist->nrecords <= 24)
		svg("<circle class=\"point\" cx=\"%.1f%%\" cy=\"%.1f%%\" r=\"4\"/>",
		    p.x, p.y);
}

static void print_points(struct graph *graph, struct curve *curve)
{
	struct record *rec;

	assert(curve != NULL);

	svg("<!-- Points -->");
	svg("<g>");

	for (rec = graph->hist->first; rec; rec = rec->next) {
		if (rec->prev)
			svg("");
		print_point(graph, curve, rec);
	}

	svg("</g>");
}

static void print_curve(struct graph *graph, struct curve *curve)
{
	print_path(graph, curve);
	svg("");
	print_points(graph, curve);
}

static void print_notice_empty(struct graph *graph)
{
	assert(graph != NULL);

	svg("<text x=\"50%%\" y=\"50%%\" style=\"font-size: 0.9em; text-anchor: middle;\">No data available</text>");
}

static void print_css(struct graph *graph)
{
	/*
	 * We want to show label when mouse is in the previous .zone
	 * rectangle.  Since label come after the rectangle, when the
	 * mouse is over the label, the rectangle will not be hovered
	 * anymore and then the label will be hidden.
	 *
	 * To avoid that, we just show label that are hovered too.
	 *
	 * Let's also note that without pointer-events: all, mouseover
	 * and mouseout events will not be triggered.
	 */
	svg("<style>");
	css(".point {");
	css("fill: #970;");
	css("}");
	css("");
	css(".labels {");
	css("visibility: hidden;");
	css("}");
	css(".labels:hover {");
	css("visibility: visible;");
	css("}");
	css(".labels > line {");
	css("stroke: #bbb;");
	css("}");
	css(".labels circle {");
	css("fill: #725800;");
	css("}");
	css(".labels text {");
	css("font-size: 0.9em;");
	css("}");
	css("");
	css(".labels > text.top_left {");
	css("text-anchor: end;");
	css("transform: translate(-10px, -11px);");
	css("}");
	css(".labels > text.top_right {");
	css("text-anchor: start;");
	css("transform: translate(10px, -11px);");
	css("}");
	css(".labels > text.bottom_left {");
	css("text-anchor: end;");
	css("transform: translate(-10px, 18px);");
	css("}");
	css(".labels > text.bottom_right {");
	css("text-anchor: start;");
	css("transform: translate(10px, 18px);");
	css("}");
	css("");
	css(".zone {");
	css("fill: none;");
	css("pointer-events: all;");
	css("}");
	css(".zone:hover + .labels {");
	css("visibility: visible;");
	css("}");
	svg("</style>");
}

static int is_empty(struct graph *graph)
{
	return graph->hist->nrecords == 0;
}

static void print_graph(struct graph *graph)
{
	unsigned i;

	assert(graph != NULL);

	svg("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	svg("<svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\" style=\"font-family: Verdana,Arial,Helvetica,sans-serif;\">");

	if (is_empty(graph)) {
		print_notice_empty(graph);
	} else {
		print_css(graph);
		svg("");
		print_axes(graph);
		for (i = 0; i < graph->ncurves; i++) {
			svg("");
			print_curve(graph, &graph->curves[i]);
		}
		print_labels(graph);
	}

	svg("</svg>");
}

int main(int argc, char **argv)
{
	struct player player;
	char *name;
	struct graph graph;

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

	graph = init_graph(&player.hist);
	add_curve(&graph, elo_to_long, 0);
	add_curve(&graph, rank_to_long, 1);
	print_graph(&graph);

	return EXIT_SUCCESS;
}
