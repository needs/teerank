#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "delta.h"

int scan_delta(struct delta *delta)
{
	unsigned i;
	int ret;

	assert(delta != NULL);

	ret = scanf(" %u %d", &delta->length, &delta->elapsed);
	if (ret == EOF)
		return 0;
	assert(ret == 2);

	for (i = 0; i < delta->length; i++) {
		ret = scanf(" %ms %ms %ld %ld",
		            &delta->players[i].name,
		            &delta->players[i].clan,
		            &delta->players[i].score,
		            &delta->players[i].delta);
		assert(ret == 4);
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
