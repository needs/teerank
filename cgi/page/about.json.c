#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "config.h"
#include "info.h"
#include "json.h"

static void json_master(struct master_info *master)
{
	putchar('{');
	printf("\"node\":\"%s\",", master->node);
	printf("\"service\":\"%s\",", master->node);
	printf("\"last_seen\":\"%s\",", json_date(master->lastseen));
	printf("\"nservers\":%u", master->nservers);
	putchar('}');
}

static void json_info(struct info *info)
{
	unsigned i;

	putchar('{');

	printf("\"nplayers\":%u,", info->nplayers);
	printf("\"nclans\":%u,", info->nclans);
	printf("\"nservers\":%u,", info->nservers);
	printf("\"last_update\":\"%s\",", json_date(info->last_update));

	printf("\"nmasters\":%u,", info->nmasters);
	printf("\"masters\":[");

	for (i = 0; i < info->nmasters; i++) {
		if (i)
			putchar(',');
		json_master(&info->masters[i]);
	}

	putchar(']');
	putchar('}');
}

int main_json_about(int argc, char **argv)
{
	struct info info;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!read_info(&info))
		return EXIT_FAILURE;

	json_info(&info);

	return EXIT_SUCCESS;
}
