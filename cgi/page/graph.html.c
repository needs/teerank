#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "teerank.h"
#include "cgi.h"
#include "html.h"
#include "player.h"

#define MAX_DATA 48

struct dataset {
	unsigned ndata;

	struct data {
		time_t ts;
		long value;
	} data[MAX_DATA];

	long min, max;
};

static void dataset_append(struct dataset *ds, time_t ts, long value)
{
	struct data *data = &ds->data[ds->ndata];

	assert(ds->ndata < MAX_DATA);

	data->ts = ts;
	data->value = value;

	if (!ds->ndata) {
		ds->min = value;
		ds->max = value;
	} else if (value > ds->max) {
		ds->max = value;
	} else if (value < ds->min) {
		ds->min = value;
	}

	ds->ndata++;
}

static int fill_datasets(
	struct dataset *dselo, struct dataset *dsrank, const char *pname)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct player_record r;
	static const struct dataset DATASET_ZERO;

	const char *query =
		"SELECT" ALL_PLAYER_RECORD_COLUMNS
		" FROM ranks_historic"
		" WHERE name IS ?"
		" ORDER BY ts"
		" LIMIT ?";

	*dselo = DATASET_ZERO;
	*dsrank = DATASET_ZERO;

	foreach_player_record(query, &r, "si", pname, MAX_DATA) {
		dataset_append(dselo,  r.ts, r.elo);
		dataset_append(dsrank, r.ts, r.rank);
	}

	if (!res)
		return FAILURE;
	if (!nrow)
		return NOT_FOUND;

	return SUCCESS;
}

struct curve {
	struct dataset *ds;

	int reversed;
	const char *color, *hover_color;
	const char *name;

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
	/* Curves share axes */
	unsigned naxes;

	unsigned ncurves;
	struct curve curves[MAX_CURVES];
};

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
	struct graph *graph, struct dataset *ds, int reversed,
	const char *color, const char *hover_color, const char *name)
{
	struct curve *curve;
	long ymin, ymax;
	unsigned naxes;

	assert(graph->ncurves < MAX_CURVES);
	curve = &graph->curves[graph->ncurves];
	graph->ncurves++;

	curve->ds = ds;
	curve->reversed = reversed;
	curve->color = color;
	curve->hover_color = hover_color;
	curve->name = name;

	ymin = ds->min;
	ymax = ds->max;

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
	return curve->ymin + i * curve->gap;
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
	return pad(y, 1.0, 7.0);
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
	struct graph *graph, struct curve *curve, struct data *data)
{
	float range, value;

	range = curve->ds->ndata - 1;
	value = data - curve->ds->data;

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
	svg("<g>");

	/* Axes */
	for (i = 0; i < graph->naxes; i++) {
		struct curve *curve = &graph->curves[0];
		long data = axe_data(curve, i + 1);
		const float y = y_coord(graph, curve, data);

		/* Line */
		svg("<line class=\"axe\" x1=\"%f%%\" y1=\"%f%%\" x2=\"%f%%\" y2=\"%f%%\"/>",
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

			data = axe_data(curve, j + 1);
			y = y_coord(graph, curve, data);

			if (i % 2 == 0) {
				class = "left";
				x = 0.0;
			} else if (i % 2 == 1) {
				class = "right";
				x = 100.0;
			}

			svg("<text class=\"axe_label %S\" x=\"%f%%\" y=\"%f%%\">%I</text>",
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
	struct graph *graph, struct curve *curve, struct data *data)
{
	struct point p;

	assert(curve != NULL);
	assert(data != NULL);

	p.x = x_coord(graph, curve, data);
	p.y = y_coord(graph, curve, data->value);

	return p;
}

static void print_path(struct graph *graph, struct curve *curve)
{
	unsigned i;

	assert(curve != NULL);

	if (curve->ds->ndata < 2)
		return;

	svg("<!-- Path -->");
	svg("<svg viewBox=\"0 0 100 100\" preserveAspectRatio=\"none\">");

	/* No need to create a class for a single element */
	svg("<path style=\"fill: none; stroke: %s; stroke-width: 3px;\" d=\"", curve->color);

	for (i = 0; i < curve->ds->ndata; i++) {
		struct point p;
		char c;

		if (i == 0)
			c = 'M';
		else
			c = 'L';

		p = init_point(graph, curve, &curve->ds->data[i]);
		svg("%c %f %f", c, p.x, p.y);

	}
	svg("\" vector-effect=\"non-scaling-stroke\"/>");
	svg("</svg>");
}

static const char *point_label_pos(
	struct graph *graph, struct curve *curve,
	struct data *data, struct point p)
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
		struct data *prevdata;

		if (p.y > 100.0 - Y_MARGIN)
			return top_left;
		else if (p.y < Y_MARGIN)
			return bottom_left;

		if (data == curve->ds->data)
			return bottom_left;

		prevdata = data - 1;

		if (prevdata->value > data->value)
			return bottom_left;
		else
			return top_left;
	} else {
		struct data *nextdata;

		if (p.y > 100.0 - Y_MARGIN)
			return top_right;
		else if (p.y < Y_MARGIN)
			return bottom_right;

		if (data - curve->ds->data == curve->ds->ndata)
			return bottom_right;

		nextdata = data + 1;

		if (nextdata->value > data->value)
			return bottom_right;
		else
			return top_right;
	}
}

static void print_zone(struct graph *graph, struct data *data, struct point p)
{
	float gap, zone_start, zone_width;
	unsigned ndata;
	int isfirst, islast;

	ndata = graph->curves[0].ds->ndata;
	isfirst = data - graph->curves[0].ds->data == 0;
	islast  = data - graph->curves[0].ds->data == ndata - 1;

	if (ndata == 1)
		gap = 100.0;
	else {
		const float unpadded_gap = 100.0 / (ndata - 1);
		gap = pad_x(graph, unpadded_gap) - pad_x(graph, 0.0);
	}

	if (isfirst) {
		zone_start = 0.0;
		zone_width = pad_x(graph, 0.0) + gap / 2.0;
	} else if (islast) {
		zone_start = p.x - gap / 2.0;
		zone_width = pad_x(graph, 100.0) + gap / 2.0;
	} else {
		zone_start = p.x - gap / 2.0;
		zone_width = gap;
	}

	svg("<rect class=\"zone\" x=\"%f%%\" y=\"0%%\" width=\"%f%%\" height=\"100%%\"/>",
	    zone_start, zone_width);
}

static void print_label(
	struct point p, struct data *data, const char *label_pos, unsigned curve_index)
{
	svg("<circle class=\"curve%u_hover\" cx=\"%f%%\" cy=\"%f%%\" r=\"4\"/>",
	    curve_index, p.x, p.y);
	svg("<text class=\"%S\" x=\"%f%%\" y=\"%f%%\">%I</text>",
	    label_pos, p.x, p.y, data->value);
}

static void print_labels(struct graph *graph)
{
	const char *label_pos;
	unsigned i, j;

	for (i = 0; i < graph->curves[0].ds->ndata; i++) {
		char buf[128];
		struct point p;
		struct data *data;
		const char *class;

		data = &graph->curves[0].ds->data[i];
		p = init_point(graph, &graph->curves[0], data);

		print_zone(graph, data, p);

		svg("<g class=\"labels\">");
		svg("<line x1=\"%f%%\" y1=\"%f%%\" x2=\"%f%%\" y2=\"%f%%\"/>",
		    p.x, pad_y(graph, 0.0), p.x, 100.0);

		for (j = 0; j < graph->ncurves; j++) {
			struct curve *curve = &graph->curves[j];

			data = &graph->curves[j].ds->data[i];
			p = init_point(graph, curve, data);
			label_pos = point_label_pos(graph, curve, data, p);
			print_label(p, data, label_pos, j);
		}

		strftime(buf, sizeof(buf), "%d %b %H:%M", gmtime(&data->ts));

		if (p.x > 88.0)
			class = "right";
		else
			class = "left";

		/* X label */
		svg("<text class=\"axe_label time %S\" x=\"%f%%\" y=\"100%%\">%s</text>",
		    class, p.x, buf);

		svg("</g>");
	}
}

static void print_point(
	struct graph *graph, struct curve *curve, struct data *data)
{
	struct point p;
	int islast;

	assert(curve != NULL);
	assert(data != NULL);

	p = init_point(graph, curve, data);
	islast = data - curve->ds->data == curve->ds->ndata;

	/*
	 * Too much points in a curve reduce readability, above 24
	 * points, don't draw them anymore.
	 */
	if (islast || curve->ds->ndata <= 24)
		svg("<circle class=\"curve%u\" cx=\"%f%%\" cy=\"%f%%\" r=\"4\"/>",
		    curve - graph->curves, p.x, p.y);
}

static void print_points(struct graph *graph, struct curve *curve)
{
	unsigned i;

	assert(curve != NULL);

	svg("<!-- Points -->");
	svg("<g>");

	for (i = 0; i < curve->ds->ndata; i++) {
		if (i > 0)
			svg("");
		print_point(graph, curve, &curve->ds->data[i]);
	}

	svg("</g>");
}

static unsigned axe_before(struct curve *curve, long data)
{
	return (data - (curve->ymin)) / curve->gap;
}

static void print_name(struct graph *graph, struct curve *curve)
{
	struct data *data;
	unsigned curve_index;
	float x, y;
	const char *class;
	unsigned axe;

	curve_index = curve - graph->curves;

	if (curve_index == 0) {
		data = &curve->ds->data[0];
		x = 0.0;
		class = "left";
	} else {
		data = &curve->ds->data[curve->ds->ndata];
		x = 100.0;
		class = "right";
	}

	axe = axe_before(curve, data->value);

	y = 0;
	y += y_coord(graph, curve, axe_data(curve, axe));
	y += y_coord(graph, curve, axe_data(curve, axe + 1));
	y /= 2.0;

	svg("<text class=\"curve%u axe_label %S\" x=\"%f%%\" y=\"%f%%\" style=\"font-weight: bold;\">%s</text>",
	    curve_index, class, x, y, curve->name);
}

static void print_curve(struct graph *graph, struct curve *curve)
{
	print_path(graph, curve);
	svg("");
	print_points(graph, curve);
	svg("");
	print_name(graph, curve);
}

static void print_notice_empty(struct graph *graph)
{
	assert(graph != NULL);

	svg("<text x=\"50%%\" y=\"50%%\" style=\"font-size: 0.9em; text-anchor: middle;\">No data available</text>");
}

static void print_css(struct graph *graph)
{
	unsigned i;

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
	css("transform: translate(10px, 0px);");
	css("text-anchor: start;");
	css("}");
	css(".axe_label.right {");
	css("transform: translate(-10px, 0px);");
	css("text-anchor: end;");
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
	css("text.axe_label.time {");
	css("dominant-baseline: text-after-edge;");
	css("}");
	css("");
	css(".zone {");
	css("fill: none;");
	css("pointer-events: all;");
	css("}");
	css(".zone:hover + .labels {");
	css("visibility: visible;");
	css("}");
	css("");

	for (i = 0; i < graph->ncurves; i++) {
		css(".curve%u {", i);
		css("fill: %s;", graph->curves[i].color);
		css("}");
		css(".curve%u_hover {", i);
		css("fill: %s;", graph->curves[i].hover_color);
		css("}");
	}

	svg("</style>");
}

static int is_empty(struct graph *graph)
{
	return graph->curves[0].ds->ndata == 0;
}

static void print_graph(struct graph *graph)
{
	unsigned i;

	assert(graph != NULL);

	svg("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	svg("<svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\" style=\"font-family: Verdana,Arial,Helvetica,sans-serif;\">");

	if (is_empty(graph)) {
		print_notice_empty(graph);
		goto end;
	}

	print_css(graph);
	svg("");
	print_axes(graph);

	/*
	 * Print curves in reverse order so that the curve added first
	 * are on top of every other curves.
	 */
	for (i = 0; i < graph->ncurves; i++) {
		svg("");
		print_curve(graph, &graph->curves[graph->ncurves - i - 1]);
	}
	svg("");
	print_labels(graph);

end:
	svg("</svg>");
}

void generate_svg_graph(struct url *url)
{
	struct graph graph = { 0 };
	struct dataset dselo, dsrank;

	fill_datasets(&dselo, &dsrank, url->dirs[1]);

	add_curve(&graph, &dselo, 0, "#970", "#725800", "Elo");
	add_curve(&graph, &dsrank, 1, "#aaa", "#888", "Rank");
	print_graph(&graph);
}
