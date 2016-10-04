#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "cgi.h"
#include "index.h"
#include "page.h"

int page_clan_list_json_main(int argc, char **argv)
{
	struct index_page ipage;
	struct jfile jfile;
	struct indexed_clan *c;

	unsigned pnum;
	int ret;

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	ret = open_index_page(
		"clans_by_nmembers", &ipage, &INDEX_DATA_INFO_CLAN,
		pnum, CLANS_PER_PAGE);
	if (ret != SUCCESS)
		return ret;

	json_init(&jfile, stdout, "<stdout>");

	json_write_object_start(&jfile, NULL);
	json_write_unsigned(&jfile, "length", ipage.plen);
	json_write_array_start(&jfile, "clans");

	while ((c = index_page_foreach(&ipage, NULL))) {
		json_write_object_start(&jfile, NULL);

		json_write_string(&jfile, "name", c->name, sizeof(c->name));
		json_write_unsigned(&jfile, "nmembers", c->nmembers);

		json_write_object_end(&jfile);
	}

	json_write_object_end(&jfile);
	json_write_array_end(&jfile);

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
