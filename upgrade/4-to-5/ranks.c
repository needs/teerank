#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "4-to-5.h"
#include "config.h"
#include "player.h"

static void upgrade(const char *old, const char *new)
{
	FILE *fold = NULL, *fnew = NULL;
	unsigned nplayers;
	char name[HEXNAME_LENGTH];
	int ret;

	fold = fopen(old, "r");
	if (fold == NULL) {
		perror(old);
		goto fail;
	}
	fnew = fopen(new, "w");
	if (fnew == NULL) {
		perror(new);
		goto fail;
	}

	errno = 0;
	ret = fscanf(fold, "%u players", &nplayers);
	if (ret == EOF && errno != 0) {
		perror(old);
		goto fail;
	} else if (ret == EOF) {
		fprintf(stderr, "%s: Early end-of-file, expected header\n", old);
		goto fail;
	} else if (ret == 0) {
		fprintf(stderr, "%s: Cannot match number of players\n", old);
		goto fail;
	}

	ret = fprintf(fnew, "%u players\n", nplayers);
	if (ret < 0) {
		perror(new);
		goto fail;
	}

	goto start;
	while (ret == 1) {
		ret = fprintf(fnew, "%-*s\n", HEXNAME_LENGTH - 1, name);
		if (ret < 0) {
			perror(new);
			goto fail;
		} else if (ret < HEXNAME_LENGTH) {
			fprintf(stderr, "%s: %d of %d bytes have not been written\n",
			        new, HEXNAME_LENGTH - ret, HEXNAME_LENGTH);
			goto fail;
		}

	start:
		ret = fscanf(fold, " %32s", name);
	}

	if (ret == EOF && errno != 0) {
		perror(old);
		goto fail;
	} else if (ret == 0) {
		fprintf(stderr, "%s: Cannot match player name\n", old);
		goto fail;
	}

	fclose(fold);
	fclose(fnew);
	return;

fail:
	if (fold)
		fclose(fold);
	if (fnew)
		fclose(fnew);
	exit(EXIT_FAILURE);
}

void upgrade_ranks(void)
{
	static char old[PATH_MAX], new[PATH_MAX];
	int ret;

	if (!dbpath(old, PATH_MAX, "ranks"))
		exit(EXIT_FAILURE);
	if (!dbpath(new, PATH_MAX, "ranks.5"))
		exit(EXIT_FAILURE);

	upgrade(old, new);

	ret = rename(new, old);
	if (ret == -1) {
		fprintf(stderr, "rename(%s, %s): %s\n", new, old, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
