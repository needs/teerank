#include <limits.h>
#include <errno.h>

#include "info.h"
#include "json.h"
#include "config.h"

int read_info(struct info *info)
{
	static const struct info INFO_ZERO;
	struct jfile jfile;
	char path[PATH_MAX];
	FILE *file;
	unsigned i;

	*info = INFO_ZERO;

	if (!dbpath(path, PATH_MAX, "info"))
		return 0;

	if (!(file = fopen(path, "r"))) {
		if (errno == ENOENT)
			return 1;
		perror(path);
		return 0;
	}

	json_init(&jfile, file, path);

	json_read_object_start(&jfile, NULL);
	json_read_unsigned(&jfile, "nplayers", &info->nplayers);
	json_read_unsigned(&jfile, "nclans", &info->nclans);
	json_read_unsigned(&jfile, "nservers", &info->nservers);
	json_read_tm(&jfile, "last_update", &info->last_update);

	json_read_unsigned(&jfile, "nmasters", &info->nmasters);
	json_read_array_start(&jfile, "masters");
	for (i = 0; i < info->nmasters; i++) {
		struct master_info *m = &info->masters[i];

		json_read_object_start(&jfile, NULL);
		json_read_string(&jfile, "node", m->node, sizeof(m->node));
		json_read_string(&jfile, "service", m->service, sizeof(m->service));
		json_read_unsigned(&jfile, "nservers", &m->nservers);
		json_read_time(&jfile, "lastseen", &m->lastseen);
		json_read_object_end(&jfile);
	}
	json_read_array_end(&jfile);

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
	unsigned i;

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

	json_write_unsigned(&jfile, "nmasters", info->nmasters);
	json_write_array_start(&jfile, "masters");
	for (i = 0; i < info->nmasters; i++) {
		struct master_info *m = &info->masters[i];

		json_write_object_start(&jfile, NULL);
		json_write_string(&jfile, "node", m->node, sizeof(m->node));
		json_write_string(&jfile, "service", m->service, sizeof(m->service));
		json_write_unsigned(&jfile, "nservers", m->nservers);
		json_write_time(&jfile, "lastseen", m->lastseen);
		json_write_object_end(&jfile);
	}
	json_write_array_end(&jfile);

	json_write_object_end(&jfile);

	fclose(file);

	return !json_have_error(&jfile);
}
