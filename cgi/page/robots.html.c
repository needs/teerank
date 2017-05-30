#include <stdio.h>
#include <stdlib.h>

#include "cgi.h"

void generate_txt_robots(struct url *url)
{
	printf("Sitemap: http://%s/sitemap.xml\n", cgi_config.domain);
}
