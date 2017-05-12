#include <stdio.h>
#include <stdlib.h>

#include "cgi.h"

int main_txt_robots(struct url *url)
{
	printf("Sitemap: http://%s/sitemap.xml\n", cgi_config.domain);
	return EXIT_SUCCESS;
}
