#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "config.h"
#include "html.h"

static void start_jsondesc_table(void)
{
	html("<table class=\"jsondesc\">");
	html("<thead>");
	html("<tr>");
	html("<th>Field</th>");
	html("<th>Type</th>");
	html("<th>Example</th>");
	html("<th>Description</th>");
	html("</tr>");
	html("</thead>");

	html("<tbody>");
}

static void jsondesc_row(
	const char *name, const char *type,
	const char *example, const char *desc)
{
	static unsigned indentlvl = 0;

	assert(name != NULL);

	if (*name == '}' || *name == ']')
		indentlvl--;

	if (*name == '{' || *name == '}' || *name == '[' || *name == ']') {
		html("<tr><td class=\"i%u\"><code>%s</code></td><td></td><td></td><td></td></tr>", indentlvl, name);
	} else {
		html("<tr>");

		if (*name)
			html("<td class=\"i%u\"><code>\"%s\":</code></td>", indentlvl, name);
		else
			html("<td></td>");

		if (strcmp(type, "hexstring") == 0)
			html("<td><code><a href=\"#hexstring\">%s</a></code></td>", type);
		else if (strcmp(type, "time") == 0)
			html("<td><code><a href=\"#time\">%s</a></code></td>", type);
		else
			html("<td><code>%s</code></td>", type);
		html("<td><code>%s</code></td>", example);
		html("<td>%s</td>", desc);
		html("</tr>");
	}

	if (*name == '{' || *name == '[')
		indentlvl++;
}

static void end_jsondesc_table(void)
{
	html("</tbody>");
	html("</table>");
}

static void jsonurl(const char *url)
{
	html("<p>");
	html("GET");
	html("<code class=\"jsonurl\">http://%s/%s</code>", cgi_config.domain, url);
	html("</p>");
}

int page_about_json_api_main(int argc, char **argv)
{
	CUSTOM_TAB.name = "JSON API Overview";
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, "JSON API Overview", NULL);

	/* Index */

	html("<h1>Index</h1>");

	html("<ul id=\"index\">");
	html("<li>");

	html("<a href=\"#custom_types\">Custom types</a>");
	html("<ul>");
	html("<li><a href=\"#hexstring\">hexstring</a></li>");
	html("<li><a href=\"#time\">time</a></li>");
	html("</ul>");

	html("</li>");

	html("<li><a href=\"#player\">Player</a></li>");
	html("<li><a href=\"#player-list\">Player list</a></li>");
	html("<li><a href=\"#clan\">Clan</a></li>");
	html("<li><a href=\"#clan-list\">Clan list</a></li>");
	html("<li><a href=\"#server-list\">Server list</a></li>");
	html("</ul>");

	html("<p>Teerank provide a JSON API for any purpose.  You are free to use it as much as you want.</p>");
	html("<p>If you find any bug, regression or typos in the following documentation, fix it or create an issue in the <a href=\"https://github.com/needs/teerank/issues\">github issue tracker</a>.  This API may change but backward compatibility will be kept at all cost.  Hence, if something break on your side after a teerank update, open an issue and it will be fixed as soon as possible.</p>");
	html("<p>Our results are compressed, however you can use the following command line to get a nicely formated result:</p>");
	html("<code>curl &lt;addr&gt; | python -m json.tool</code>");
	html("<p>Use a robust JSON parser and always check the result of your requests, escpecially string size.  It is possible that our implementation is buggy or even worse, someone comprised teerank and try to harm you.</p>");

	/* Custom types */

	html("<h1 id=\"custom-types\">Custom types</h1>");

	html("<h2 id=\"hexstring\"><code>hexstring</code></h2>");

	html("<p><code>hexstring</code> stands for \"hexadecimal string\", it is a regular json string, but only contains hexadecimal characters.  Hexstrings can be converted to regular string.  The purpose of using hexstrings is to avoid forbidden chars in filenames or in URLs.</p>");

	html("<p>Hexstrings should always terminate with <code>\"00\"</code>.  Converting an hexstring to a regular string is done by converting each pair of hexadecimal digit to a byte.  For instance, <code>\"6e616d656c6573732074656500\"</code> is the hexstring for <code>\"nameless tee\\0\"</code>, where <code>\"6e\"</code> convert to <code>\"n\"</code>, ...</p>");

	html("<h2 id=\"time\"><code>time</code></h2>");

	html("<p>Time are encoded using the standard <a href=\"https://www.ietf.org/rfc/rfc3339.txt\">RFC-3339</a>.  It provide numerous advantages: fixed length, human readable, and is supported on many platforms and libraries.  They looks like the following: <code>\"2016-01-19T22:42:31Z\"</code></p>");

	html("<p>The lowest possible date is <code>\"1970-01-01T00:00:00Z\"</code>, or unix epoch.</p>");

	/*
	 * Player
	 */

	html("<h1 id=\"player\">Player</h1>");

	jsonurl("players/<em>player_id</em>.json");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"6e616d656c6573732074656500\"", "Player name");
	jsondesc_row("clan", "hexstring", "\"00\"", "Player clan");
	jsondesc_row("elo", "integer", "1500", "Player elo points");
	jsondesc_row("rank", "unsigned", "45678", "Player rank");
	jsondesc_row("last_seen", "time", "\"1970-01-01T00:00:00Z\"", "Last time the player was connected");
	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	/*
	 * Player list
	 */

	html("<h1 id=\"player-list\">Player list</h1>");

	jsonurl("players/by-rank.json?p=<em>page_number</em>");
	jsonurl("players/by-lastseen.json?p=<em>page_number</em>");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("length", "unsigned", "1", "Number of players in the following array");

	jsondesc_row("players", "", "", "Array of <code>length</code> players");
	jsondesc_row("[", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"6e616d656c6573732074656500\"", "Player name");
	jsondesc_row("clan", "hexstring", "\"00\"", "Player clan");
	jsondesc_row("elo", "integer", "1500", "Player elo points");
	jsondesc_row("rank", "unsigned", "45678", "Player rank");
	jsondesc_row("last_seen", "time", "\"1970-01-01T00:00:00Z\"", "Last time the player was connected");
	jsondesc_row("]", NULL, NULL, NULL);

	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	/*
	 * Clan
	 */

	html("<h1 id=\"clan\">Clan</h1>");

	jsonurl("clans/<em>clan_id</em>.json");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"00\"", "Clan name");
	jsondesc_row("nmembers", "unsigned", "3", "Number of players in the clan");

	jsondesc_row("members", "", "", "Array of <code>nmembers</code> players name");
	jsondesc_row("[", NULL, NULL, NULL);
	jsondesc_row("", "hexstring", "\"6e616d656c6573732074656500\"", "Player name");
	jsondesc_row("]", NULL, NULL, NULL);

	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	/*
	 * Clan list
	 */

	html("<h1 id=\"clan-list\">Clan list</h1>");

	jsonurl("clans/by-nmembers.json?p=<em>page_number</em>");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("length", "unsigned", "1", "Number of clan names in the following array");

	jsondesc_row("clans", "", "", "Array of <code>length</code> clan names");
	jsondesc_row("[", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"00\"", "Clan name");
	jsondesc_row("nmembers", "unsigned", "2", "Number of members");
	jsondesc_row("]", NULL, NULL, NULL);

	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	/*
	 * Server list
	 */

	html("<h1 id=\"server-list\">Server list</h1>");

	jsonurl("servers/by-nplayers.json?p=<em>page_number</em>");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("length", "unsigned", "3", "Number of servers in the following array");

	jsondesc_row("servers", "", "", "Array of <code>length</code> servers");
	jsondesc_row("[", NULL, NULL, NULL);
	jsondesc_row("name", "string", "\"[xyz] servers\"", "Server name");
	jsondesc_row("gametype", "string", "\"CTF\"", "Server gametype");
	jsondesc_row("map", "string", "\"ctf1\"", "Server map");
	jsondesc_row("nplayers", "unsigned", "5", "Number of players in the server");
	jsondesc_row("maxplayers", "unsigned", "16", "Maximum number of players");
	jsondesc_row("]", NULL, NULL, NULL);

	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	html_footer(NULL);

	return EXIT_SUCCESS;
}
