#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cgi.h"
#include "teerank.h"
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

int main_html_about_json_api(struct url *url)
{
	html_header("JSON API Overview", "JSON API Overview", "", NULL);

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

	html("<li><a href=\"#about\">About</a></li>");
	html("<li><a href=\"#player\">Player</a></li>");
	html("<li><a href=\"#player-list\">Player list</a></li>");
	html("<li><a href=\"#clan\">Clan</a></li>");
	html("<li><a href=\"#clan-list\">Clan list</a></li>");
	html("<li><a href=\"#server\">Server</a></li>");
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

	html("<p>The lowest possible date is <code>\"1970-01-01T00:00:00Z\"</code>, or unix epoch.  This date is used to mark the absence of meaningful value.</p>");

	/*
	 * About
	 */

	html("<h1 id=\"about\">About</h1>");

	jsonurl("about.json");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("nplayers", "unsigned", "\"314\"", "Number of players in the database");
	jsondesc_row("nclans", "unsigned", "\"271\"", "Number of clans in the database");
	jsondesc_row("nservers", "unsigned", "\"1000\"", "Number of servers in the database");
	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

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
	jsondesc_row("lastseen", "time", "\"1970-01-01T00:00:00Z\"", "Last time the player was connected");
	jsondesc_row("server_ip", "string", "\"1.2.3.4\"", "Last server IP player was connected to");
	jsondesc_row("server_port", "string", "\"8300\"", "Last server port player was connected to");

	jsondesc_row("historic", "", "", "Historic of elo and rank values");
	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("length", "unsigned", "2", "Number fo records");
	jsondesc_row("epoch", "time", "2016-10-22T01:46:00Z", "Creation date of the historic");
	jsondesc_row("records", "", "", "Array of records");
	jsondesc_row("[", NULL, NULL, NULL);
	jsondesc_row("", "unsigned", "0", "Elapsed time since epoch");
	jsondesc_row("", "integer", "1500", "Player elo points");
	jsondesc_row("", "unsigned", "45678", "Player rank");
	jsondesc_row("]", NULL, NULL, NULL);
	jsondesc_row("}", NULL, NULL, NULL);

	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	jsonurl("players/<em>player_id</em>.json?short");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"6e616d656c6573732074656500\"", "Player name");
	jsondesc_row("clan", "hexstring", "\"00\"", "Player clan");
	jsondesc_row("elo", "integer", "1500", "Player elo points");
	jsondesc_row("rank", "unsigned", "45678", "Player rank");
	jsondesc_row("lastseen", "time", "\"1970-01-01T00:00:00Z\"", "Last time the player was connected");
	jsondesc_row("server_ip", "string", "\"1.2.3.4\"", "Last server IP player was connected to");
	jsondesc_row("server_port", "string", "\"8300\"", "Last server port player was connected to");
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
	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"6e616d656c6573732074656500\"", "Player name");
	jsondesc_row("clan", "hexstring", "\"00\"", "Player clan");
	jsondesc_row("elo", "integer", "1500", "Player elo points");
	jsondesc_row("rank", "unsigned", "45678", "Player rank");
	jsondesc_row("lastseen", "time", "\"1970-01-01T00:00:00Z\"", "Last time the player was connected");
	jsondesc_row("server_ip", "string", "\"1.2.3.4\"", "IP of the last server the player was playing on (can be empty)");
	jsondesc_row("server_port", "string", "\"8300\"", "Port of the last server the player was playing on (can be empty)");
	jsondesc_row("}", NULL, NULL, NULL);
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
	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"00\"", "Clan name");
	jsondesc_row("nmembers", "unsigned", "2", "Number of members");
	jsondesc_row("}", NULL, NULL, NULL);
	jsondesc_row("]", NULL, NULL, NULL);

	jsondesc_row("}", NULL, NULL, NULL);

	end_jsondesc_table();

	/*
	 * Server
	 */

	html("<h1 id=\"server\">Server</h1>");

	jsonurl("servers/<em>server_addr</em>.json");

	start_jsondesc_table();

	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("ip", "hexstring", "\"192.168.0.1\"", "Server IP (Either IPv4 or IPv6)");
	jsondesc_row("port", "hexstring", "\"8300\"", "Server port");
	jsondesc_row("name", "hexstring", "\"Vanilla CTF5\"", "Server name");
	jsondesc_row("gametype", "hexstring", "\"CTF\"", "Server gametype");
	jsondesc_row("map", "hexstring", "\"ctf5\"", "Server map");
	jsondesc_row("lastseen", "time", "\"2016-10-01T19:33:01Z\"", "Last time the server was succesfully contacted");
	jsondesc_row("expire", "time", "\"2016-10-01T19:33:60Z\"", "Next time the server will be contacted");
	jsondesc_row("num_clients", "unsigned", "3", "Number of players in the server (including spectators)");
	jsondesc_row("max_clients", "unsigned", "16", "Maximum number of players (including spectators)");

	jsondesc_row("clients", "", "", "Array of <code>num_clients</code> players");
	jsondesc_row("[", NULL, NULL, NULL);
	jsondesc_row("{", NULL, NULL, NULL);
	jsondesc_row("name", "hexstring", "\"6e616d656c6573732074656500\"", "Player name");
	jsondesc_row("clan", "hexstring", "\"00\"", "Player clan");
	jsondesc_row("score", "int", "5", "Actual player score (spectator also have a score)");
	jsondesc_row("ingame", "bool", "true", "false if spectating, true if playing");
	jsondesc_row("}", NULL, NULL, NULL);
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

	html_footer(NULL, NULL);

	return EXIT_SUCCESS;
}
