#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "delta.h"

int scan_delta(struct delta *delta)
{
	unsigned i;
	int ret;

	assert(delta != NULL);

	errno = 0;
	ret = scanf(" %u %d", &delta->length, &delta->elapsed);
	if (ret == EOF && errno == 0)
		return 0;
	else if (ret == EOF && errno != 0)
		return perror("<stdin>"), 0;
	else if (ret == 0)
		return fprintf(stderr, "<stdin>: Cannot match delta length\n"), 0;
	else if (ret == 1)
		return fprintf(stderr, "<stdin>: Cannot match delta elapsed time\n"), 0;

	for (i = 0; i < delta->length; i++) {
		errno = 0;
		ret = scanf(" %s %s %ld %ld",
		            delta->players[i].name,
		            delta->players[i].clan,
		            &delta->players[i].score,
		            &delta->players[i].delta);
		if (ret == EOF && errno == 0)
			return fprintf(stderr, "<stdin>: Expected %u players, found %u\n", delta->length, i), 0;
		else if (ret == EOF && errno != 0)
			return perror("<stdin>"), 0;
		else if (ret == 0)
			return fprintf(stderr, "<stdin>: Cannot match player name\n"), 0;
		else if (ret == 1)
			return fprintf(stderr, "<stdin>: Cannot match player clan\n"), 0;
		else if (ret == 2)
			return fprintf(stderr, "<stdin>: Cannot match player score\n"), 0;
		else if (ret == 3)
			return fprintf(stderr, "<stdin>: Cannot match player delta\n"), 0;

		if (!is_valid_hexname(delta->players[i].name))
			return fprintf(stderr, "<stdin>: %s: Not a valid player name\n", delta->players[i].name), 0;
		if (!is_valid_hexname(delta->players[i].clan))
			return fprintf(stderr, "<stdin>: %s: Not a valid player clan\n", delta->players[i].clan), 0;
	}

	return 1;
}

void print_delta(struct delta *delta)
{
	if (delta->length) {
		unsigned i;

		printf("%u %d\n", delta->length, delta->elapsed);

		for (i = 0; i < delta->length; i++) {
			printf("%s %s %ld %ld\n",
			       delta->players[i].name,
			       delta->players[i].clan,
			       delta->players[i].score,
			       delta->players[i].delta);
		}
	}
}
