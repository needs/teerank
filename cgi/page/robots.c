#include <stdio.h>
#include <stdlib.h>

#include "cgi.h"

int page_robots_main(int argc, char **argv)
{
	printf("Sitemap: http://%s/sitemap.xml\n", cgi_config.domain);
	return EXIT_SUCCESS;
}
