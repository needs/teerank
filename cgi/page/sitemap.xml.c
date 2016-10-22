#include <stdio.h>
#include <stdlib.h>

#include "cgi.h"
#include "html.h"

int main_xml_sitemap(int argc, char **argv)
{
	xml("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	xml("<urlset");
	xml(" xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\"");
	xml(" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
	xml(" xsi:schemaLocation=\"http://www.sitemaps.org/schemas/sitemap/0.9 http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd\">");

	xml("<url>");
	xml("<loc>http://%s/players</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("<url>");
	xml("<loc>http://%s/clans</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("<url>");
	xml("<loc>http://%s/servers</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("<url>");
	xml("<loc>http://%s/about</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("</urlset>");

	return EXIT_SUCCESS;
}
