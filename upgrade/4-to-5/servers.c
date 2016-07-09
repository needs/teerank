/*
 * Concatenate "meta" file and "state" file for each servers.  Then
 * remove them and also the parent directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>

#include "4-to-5.h"
#include "config.h"

static int remove_server_file(const char *sname, const char *filename)
{
	char path[PATH_MAX];
	int ret;

	ret = snprintf(path, PATH_MAX, "%s/servers/%s/%s",
	               config.root, sname, filename);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		return 0;
	}

	ret = unlink(path);
	if (ret == -1 && errno != ENOENT) {
		perror(path);
		return 0;
	}

	return 1;
}

/*
 * Return a path to the removed directory
 */
static void remove_server_directory(const char *sname)
{
	char server_dir[PATH_MAX];
	int ret;

	if (!remove_server_file(sname, "state"))
		goto fail;
	if (!remove_server_file(sname, "meta"))
		goto fail;

	/*
	 * "infos" files should not exist anymore, however there was a
	 * way in previous upgrade scripts that an "infos" file will not
	 * be deleted.  Instead of ignoring it and failing at rmdir(),
	 * delete it.
	 */
	if (!remove_server_file(sname, "infos"))
		goto fail;

	ret = snprintf(server_dir, PATH_MAX, "%s/servers/%s",
	               config.root, sname);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	if (rmdir(server_dir) == -1) {
		perror(server_dir);
		goto fail;
	}

	return;
fail:
	exit(EXIT_FAILURE);
}

/*
 * Remove server directory and replace it by file located at "src_path".
 */
static void commit_upgrade(const char *sname, const char *src_path)
{
	char server_dir[PATH_MAX];
	int ret;

	ret = snprintf(server_dir, PATH_MAX, "%s/servers/%s",
	               config.root, sname);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	if (rename(src_path, server_dir) == -1) {
		fprintf(stderr, "rename(%s, %s): %s\n",
		        src_path, server_dir, strerror(errno));
		goto fail;
	}

	return;
fail:
	exit(EXIT_FAILURE);
}

static int upgrade_server(const char *sname)
{
	static char dst_path[PATH_MAX];
	char meta_path[PATH_MAX], state_path[PATH_MAX];
	FILE *fstate = NULL, *fmeta = NULL, *fdst = NULL;
	int c, ret;

	ret = snprintf(meta_path, PATH_MAX, "%s/servers/%s/meta",
	               config.root, sname);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	ret = snprintf(state_path, PATH_MAX, "%s/servers/%s/state",
	               config.root, sname);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	ret = snprintf(dst_path, PATH_MAX, "%s/servers/%s.5",
	               config.root, sname);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	/*
	 * No meta file or no state file is not fatal but this
	 * particular server will not be upgraded.  We could make an
	 * empty state or create metadata now, but that would be too
	 * too complex for what it would result.
	 */

	fmeta = fopen(meta_path, "r");
	if (!fmeta && errno != ENOENT) {
		perror(meta_path);
		goto fail;
	} else if (!fmeta && errno == ENOENT) {
		remove_server_directory(sname);
		return 0;
	}

	fstate = fopen(state_path, "r");
	if (!fstate && errno != ENOENT) {
		perror(state_path);
		goto fail;
	} else if (!fstate && errno == ENOENT) {
		fclose(fmeta);
		remove_server_directory(sname);
		return 0;
	}

	fdst = fopen(dst_path, "w");
	if (!fdst) {
		perror(dst_path);
		goto fail;
	}

	while ((c = fgetc(fmeta)) != EOF)
		fputc(c, fdst);

	if (fstate) {
		fputc('\n', fdst);
		while ((c = fgetc(fstate)) != EOF)
			fputc(c, fdst);
	}

	fclose(fmeta);
	fclose(fdst);
	if (fstate)
		fclose(fstate);

	remove_server_directory(sname);
	commit_upgrade(sname, dst_path);

	return 1;

fail:
	if (fmeta)
		fclose(fmeta);
	if (fstate)
		fclose(fstate);
	if (fdst)
		fclose(fdst);
	exit(EXIT_FAILURE);
}

void upgrade_servers(void)
{
	char path[PATH_MAX];
	struct dirent *dp;
	DIR *dir = NULL;
	int ret;

	ret = snprintf(path, PATH_MAX, "%s/servers", config.root);
	if (ret >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		goto fail;
	}

	dir = opendir(path);
	if (!dir) {
		perror(path);
		goto fail;
	}

	while ((dp = readdir(dir))) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		upgrade_server(dp->d_name);
	}

	closedir(dir);
	return;

fail:
	if (dir)
		closedir(dir);
	exit(EXIT_FAILURE);
}
