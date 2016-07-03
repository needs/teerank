#include <stdlib.h>
#include <stdio.h>

#include "config.h"

int main(int argc, char *argv[])
{
#define STRING(envname, val, name) \
	printf("if [ -z \"$%s\" ]; then %s=%s; fi\n", envname, envname, val);
#define BOOL(envname, val, name)	\
	printf("if [ -z \"$%s\" ]; then %s=%d; fi\n", envname, envname, val);
#include "default_config.h"
#undef BOOL
#undef STRING
	return EXIT_SUCCESS;
}
