#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define MAX_NAME_LENGTH 32

static void print_file(char *path)
{
	FILE *file = NULL;
	int c;

	if (!(file = fopen(path, "r")))
		exit(EXIT_FAILURE);

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
}

struct player {
	char name[MAX_NAME_LENGTH];
	int elo;
};

static void hex_to_string(char *hex, char *name)
{
	assert(hex != NULL);
	assert(name != NULL);

	for (; hex[0] != '0' || hex[1] != '0'; hex += 2, name++) {
		char tmp[3] = { hex[0], hex[1], '\0' };
		*name = strtol(tmp, NULL, 16);
	}

	*name = '\0';
}

static void html(char *str)
{
	assert(str != NULL);

	do {
		switch (*str) {
		case '<':
			fputs("&lt;", stdout); break;
		case '>':
			fputs("&gt;", stdout); break;
		case '&':
			fputs("&amp;", stdout); break;
		case '"':
			fputs("&quot;", stdout); break;
		default:
			putchar(*str);
		}
	} while (*str++);
}

static char *get_path(char *dir, char *player, char *filename)
{
	static char path[PATH_MAX];

	assert(strlen(dir) + strlen(player) + strlen(filename) + 2 < PATH_MAX);

	sprintf(path, "%s/%s/%s", dir, player, filename);
	return path;
}

static int read_elo(char *dir, char *player, int *elo)
{
	FILE *file;
	char *path;

	assert(dir != NULL);
	assert(player != NULL);
	assert(elo != NULL);

	path = get_path(dir, player, "elo");

	file = fopen(path, "r");
	if (!file)
		return perror(path), 0;

	if (fscanf(file, "%d", elo) == 1)
		return fclose(file), 1;
	else
		return fclose(file), 0;
}

struct player_array {
	struct player* players;
	unsigned length;
};

static void add_player(struct player_array *array, struct player *player)
{
	static const unsigned OFFSET = 1024;

	assert(array != NULL);
	assert(player != NULL);

	if (array->length % 1024 == 0) {
		array->players = realloc(array->players, (array->length + OFFSET) * sizeof(*player));
		if (!array->players)
			perror("Allocating player array"), exit(EXIT_FAILURE);
	}

	array->players[array->length++] = *player;
}

static void load_players(char *path, struct player_array *array)
{
	DIR *dir;
	struct dirent *dp;

	if (!(dir = opendir(path)))
		perror(path), exit(EXIT_FAILURE);

	while ((dp = readdir(dir))) {
		struct player player;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		hex_to_string(dp->d_name, player.name);
		read_elo(path, dp->d_name, &player.elo);

		add_player(array, &player);
	}

	closedir(dir);
}

static int cmp_player(const void *p1, const void *p2)
{
	const struct player *a = p1, *b = p2;

	/* We want them in reverse order */
	return b->elo - a->elo;
}

static const struct player_array PLAYER_ARRAY_ZERO;

int main(int argc, char *argv[])
{
	struct player_array array = PLAYER_ARRAY_ZERO;
	unsigned i;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <player_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	load_players(argv[1], &array);
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	print_file("html/header.inc.html");
	for (i = 0; i < array.length; i++) {
		printf("<tr><td>%u</td><td>", i + 1);
		html(array.players[i].name);
		printf("</td><td></td><td>%d</td></tr>\n", array.players[i].elo);
	}
	print_file("html/footer.inc.html");

	return EXIT_SUCCESS;
}
