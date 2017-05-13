#include <stdio.h>
#include <stdlib.h>

#include "cgi.h"
#include "html.h"

int main_xml_sitemap(struct url *url)
{
	xml("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	xml("<urlset");
	xml(" xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\"");
	xml(" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
	xml(" xsi:schemaLocation=\"http://www.sitemaps.org/schemas/sitemap/0.9 http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd\">");

	xml("<url>");
	xml("<loc>http://%S/players</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("<url>");
	xml("<loc>http://%S/clans</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("<url>");
	xml("<loc>http://%S/servers</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("<url>");
	xml("<loc>http://%S/about</loc>", cgi_config.domain);
	xml("<priority>1.00</priority>");
	xml("<changefreq>hourly</changefreq>");
	xml("</url>");

	xml("</urlset>");

	return EXIT_SUCCESS;
}
