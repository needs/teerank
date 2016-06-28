#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "html.h"
#include "player.h"

enum timescale {
	BY_YEAR, LAST_MONTH, TODAY
};

static const char *timescale_string(enum timescale ts)
{
	switch (ts) {
	case BY_YEAR:
		return "By year";
	case LAST_MONTH:
		return "Last month";
	case TODAY:
		return "Today";
	default:
		return NULL;
	}
}

static const char *timescale_param(enum timescale ts)
{
	switch (ts) {
	case BY_YEAR:
		return "monthly";
	case LAST_MONTH:
		return "daily";
	case TODAY:
		return "hourly";
	default:
		return NULL;
	}
}

static unsigned timescale_nrecords(struct player *player, enum timescale ts)
{
	switch (ts) {
	case BY_YEAR:
		return player->monthly_rank.nrecords;
	case LAST_MONTH:
		return player->daily_rank.nrecords;
	case TODAY:
		return player->hourly_rank.nrecords;
	default:
		return 0;
	}
}

static void print_timescale(struct player *player, enum timescale active)
{
	enum timescale ts;

	html("<ul id=\"timescale\">");
	for (ts = BY_YEAR; ts <= TODAY; ts++) {
		const char *class;
		int href;

		if (ts == active) {
			href = 0;
			class = "active";
		} else if (timescale_nrecords(player, ts) <= 2) {
			href = 0;
			class = "disabled";
		} else {
			href = 1;
			class = "available";
		}

		if (href) {
			html("<li><a class=\"%s\" href=\"/players/%s.html?%s\">%s</a></li>",
			     class, player->name,
			     timescale_param(ts), timescale_string(ts));
		} else {
			html("<li><a class=\"%s\">%s</a></li>",
			     class, timescale_string(ts));
		}
	}
	html("</ul>");
}

static enum timescale parse_timescale(const char *str)
{
	if (strcmp(str, "hourly") == 0)
		return TODAY;
	else if (strcmp(str, "daily") == 0)
		return LAST_MONTH;
	else if (strcmp(str, "monthly") == 0)
		return BY_YEAR;
	else {
		fprintf(stderr, "\"%s\" is not a valid timescale, expected \"hourly\", \"daily\" or \"monthly\"\n", str);
		exit(EXIT_FAILURE);
	}
}

static char *parse_player_hexname(const char *str)
{
	if (!is_valid_hexname(str)) {
		fprintf(stderr, "\"%s\" is not a valid player hexadecimal name\n", str);
		exit(EXIT_NOT_FOUND);
	}

	return (char*)str;
}

int main(int argc, char **argv)
{
	char name[HEXNAME_LENGTH], clan[NAME_LENGTH];
	char *hexname;
	enum timescale timescale;
	struct player player;
	int player_found;

	load_config();
	if (argc != 3) {
		fprintf(stderr, "usage: %s <player_name> hourly|daily|monthly\n", argv[0]);
		return EXIT_FAILURE;
	}

	hexname = parse_player_hexname(argv[1]);
	timescale = parse_timescale(argv[2]);

	init_player(&player);
	player_found = read_player(&player, hexname);

	hexname_to_name(hexname, name);
	CUSTOM_TAB.name = name;
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, name, NULL);

	/* Print player logo, name, clan, rank and elo */
	if (player_found) {
		hexname_to_name(player.clan, clan);
		html("<header id=\"player_header\">");
		html("<img src=\"/images/player.png\"/>");
		html("<section>");
		html("<h1>%s</h1>", name);
		html("<p>%s</p>", clan);
		html("</section>");
		html("<p>#%u (%d ELO)</p>", player.rank, player.elo);
		html("</header>");
		html("");
		html("<h2>ELO points (%d)</h2>", player.elo);
		html("<object data=\"/players/%s/elo.svg\" type=\"image/svg+xml\"></object>", player.name);
		html("");
		html("<section>");
		html("<header class=\"with_timescale\">");
		html("<h2>Rank (%u)</h2>", player.rank);
		print_timescale(&player, timescale);
		html("</header>");
		html("<object data=\"/players/%s/rank.svg?%s\" type=\"image/svg+xml\"></object>",
		     player.name, timescale_param(timescale));
		html("</section>");
	} else {
		html("<p>Player not found</p>");
	}

	html_footer();

	return EXIT_SUCCESS;
}
