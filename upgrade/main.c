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

static void read_old_player(sqlite3_stmt *res, void *_p)
{
	struct old_player *p = _p;

	p->name = (char*)sqlite3_column_text(res, 0);
	p->clan = (char*)sqlite3_column_text(res, 1);

	p->elo = sqlite3_column_int(res, 2);
	p->rank = sqlite3_column_int64(res, 3);

	p->lastseen = sqlite3_column_int64(res, 4);
	p->server_ip = (char*)sqlite3_column_text(res, 5);
	p->server_port = (char*)sqlite3_column_text(res, 6);
}

static int upgrade_player(struct old_player *oldp)
{
	const char *query =
		"INSERT INTO _players"
		" VALUES(?, ?, ?, ?, ?)";

#define bind_old_player(p) "sstss", \
	(p)->name, (p)->clan, (p)->lastseen, (p)->server_ip, (p)->server_port

	return exec(query, bind_old_player(oldp));
}

static void upgrade_players(void)
{
	int ret = 0;
	unsigned nrow;
	sqlite3_stmt *res;

	struct old_player oldp;

	const char *select_old_players =
		"SELECT *"
		" FROM players";

	const char *add_rank =
		"INSERT INTO ranks"
		" VALUES(?, ?, ?, ?, ?)";

	if (!exec("CREATE TABLE _players" TABLE_DESC_PLAYERS))
		exit(EXIT_FAILURE);

	foreach_row(select_old_players, read_old_player, &oldp) {
		ret |= upgrade_player(&oldp);

#define bind_rank(oldp, gametype) "sssiu", \
	(oldp).name, (gametype), "", (oldp).elo, (oldp).rank

		/* Since we upgraded all players, ranks remain meaningful */
		ret |= exec(add_rank, bind_rank(oldp, "CTF"));
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

static void read_pending(sqlite3_stmt *res, void *_p)
{
	struct pending *p = _p;
	p->name = (char*)sqlite3_column_text(res, 0);
	p->elo = sqlite3_column_int(res, 1);
}

/* "pending" table now have two new columns: "gametype" and "maps" */
static void upgrade_pending(void)
{
	unsigned nrow;
	sqlite3_stmt *res;
	struct pending p;

	const char *select_pendings =
		"SELECT name, elo FROM pending";

	const char *add_pending =
		"INSERT INTO _pending VALUES(?, 'CTF', '', ?)";

	if (!exec("CREATE TABLE _pending" TABLE_DESC_PENDING))
		return;

	foreach_row(select_pendings, read_pending, &p)
		exec(add_pending, "si", p.name, p.elo);

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
