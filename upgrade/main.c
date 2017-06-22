#include <stdlib.h>
#include <stdio.h>

#include "database.h"
#include "teerank.h"

struct old_player {
	const char *name;
	const char *clan;
	int elo;
	unsigned rank;
	time_t lastseen;
	const char *server_ip;
	const char *server_port;
};

static void read_old_player(sqlite3_stmt *res, struct old_player *p)
{
	p->name = column_text(res, 0);
	p->clan = column_text(res, 1);

	p->elo = column_int(res, 2);
	p->rank = column_unsigned(res, 3);

	p->lastseen = column_time_t(res, 4);
	p->server_ip = column_text(res, 5);
	p->server_port = column_text(res, 6);
}

static bool upgrade_player(struct old_player *p)
{
	const char *query =
		"INSERT INTO _players"
		" VALUES(?, ?, ?, ?, ?)";

	return exec(
		query, "sstss", p->name, p->clan,
		p->lastseen, p->server_ip, p->server_port);
}

static void upgrade_players(void)
{
	int ret = 0;
	sqlite3_stmt *res;

	const char *add_rank =
		"INSERT INTO ranks"
		" VALUES(?, 'CTF', NULL, ?, ?)";

	if (!exec("CREATE TABLE _players" TABLE_DESC_PLAYERS))
		exit(EXIT_FAILURE);

	foreach_row(res, "SELECT * FROM players") {
		struct old_player oldp;
		read_old_player(res, &oldp);
		ret |= upgrade_player(&oldp);

		/* Since we upgraded all players, ranks remain meaningful */
		ret |= exec(add_rank, "siu", oldp.name, oldp.elo, oldp.rank);
	}

	ret |= exec("DROP TABLE players");
	ret |= exec("ALTER TABLE _players RENAME TO players");

	if (!res || !ret)
		exit(EXIT_FAILURE);
}

struct pending {
	const char *name;
	int elo;
};

/* "pending" table now have two new columns: "gametype" and "maps" */
static void upgrade_pending(void)
{
	sqlite3_stmt *res;
	const char *add_pending =
		"INSERT INTO _pending VALUES(?, 'CTF', NULL, ?)";

	if (!exec("CREATE TABLE _pending" TABLE_DESC_PENDING))
		return;

	foreach_row(res, "SELECT name, elo FROM pending") {
		char *name = column_text(res, 0);
		int elo = column_int(res, 1);
		exec(add_pending, "si", name, elo);
	}

	exec("DROP TABLE pending");
	exec("ALTER TABLE _pending RENAME TO pending");
}

int main(int argc, char *argv[])
{
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	init_teerank(UPGRADABLE);

	exec("BEGIN");

	drop_all_indices();

	exec("CREATE TABLE ranks" TABLE_DESC_RANKS);
	exec("CREATE TABLE ranks_historic" TABLE_DESC_RANK_HISTORIC);

	upgrade_pending();
	upgrade_players();

	create_all_indices();

	exec("UPDATE version SET version = ?", "i", DATABASE_VERSION);

	exec("COMMIT");

	return EXIT_SUCCESS;
}
