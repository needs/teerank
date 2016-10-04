#include <limits.h>

#include "info.h"
#include "json.h"
#include "config.h"

int read_info(struct info *info)
{
	struct jfile jfile;
	char path[PATH_MAX];
	FILE *file;

	if (!dbpath(path, PATH_MAX, "info"))
		return 0;

	if (!(file = fopen(path, "r"))) {
		perror(path);
		return 0;
	}

	json_init(&jfile, file, path);

	json_read_object_start(&jfile, NULL);
	json_read_unsigned(&jfile, "nplayers", &info->nplayers);
	json_read_unsigned(&jfile, "nclans", &info->nclans);
	json_read_unsigned(&jfile, "nservers", &info->nservers);
	json_read_tm(&jfile, "last_update", &info->last_update);
	json_read_object_end(&jfile);

	fclose(file);

	return !json_have_error(&jfile);
}

int write_info(struct info *info)
{
	struct jfile jfile;
	char path[PATH_MAX];
	FILE *file;
	time_t now;

	if (!dbpath(path, PATH_MAX, "info"))
		return 0;

	if (!(file = fopen(path, "w"))) {
		perror(path);
		return 0;
	}

	now = time(NULL);
	info->last_update = *gmtime(&now);

	json_init(&jfile, file, path);

	json_write_object_start(&jfile, NULL);
	json_write_unsigned(&jfile, "nplayers", info->nplayers);
	json_write_unsigned(&jfile, "nclans", info->nclans);
	json_write_unsigned(&jfile, "nservers", info->nservers);
	json_write_tm(&jfile, "last_update", info->last_update);
	json_write_object_end(&jfile);

	fclose(file);

	return !json_have_error(&jfile);
}
