#include <stdlib.h>

#include "config.h"
#include "io.h"

int main(int argc, char **argv)
{
	load_config();

	if (argc != 2) {
		fprintf(stderr, "usage: %s <query>\n", argv[0]);
		return EXIT_FAILURE;
	}

	CUSTOM_TAB.name = "Search results";
	CUSTOM_TAB.href = "";

	print_header(&CUSTOM_TAB, "Search results", argv[1]);
	print_footer();

	return EXIT_SUCCESS;
}
