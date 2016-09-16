#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "cgi.h"
#include "index.h"
#include "page.h"

int page_clan_list_json_main(int argc, char **argv)
{
	struct index_page ipage;

	unsigned pnum;
	int ret;

	if (!parse_pnum(argv[1], &pnum))
		return EXIT_NOT_FOUND;

	ret = open_index_page(
		"clans_by_nmembers", &ipage, INDEX_DATA_INFO_CLAN,
		pnum, CLANS_PER_PAGE);

	if (ret == PAGE_NOT_FOUND)
		return EXIT_NOT_FOUND;
	if (ret == PAGE_ERROR)
		return EXIT_FAILURE;

	printf("{\"length\":%u,\"clans\":[", ipage.plen);

	if (!index_page_dump_all(&ipage))
		return EXIT_FAILURE;

	printf("]}");

	close_index_page(&ipage);

	return EXIT_SUCCESS;
}
