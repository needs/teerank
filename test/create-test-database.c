#include "teerank.h"
#include "database.h"

int main(void)
{
	init_teerank(0);

	/* Players */
	exec("INSERT INTO players VALUES('tee1', '', time('now'))");
	exec("INSERT INTO players VALUES('tee2', 'clan1', time('now'))");
	exec("INSERT INTO players VALUES('tee3', 'clan1', time('now'))");

	/* Ranks for 'tee1' */
	exec("INSERT INTO ranks VALUES('tee1', 'CTF', '', 1600, 1)");
	exec("INSERT INTO ranks VALUES('tee1', 'CTF', 'ctf1', 1550, 1)");
	exec("INSERT INTO ranks VALUES('tee1', 'CTF', 'ctf2', 1650, 1)");

	/* Ranks for 'tee2' */
	exec("INSERT INTO ranks VALUES('tee2', 'CTF', '', 1500, 2)");
	exec("INSERT INTO ranks VALUES('tee2', 'CTF', 'ctf1', 1500, 2)");
	exec("INSERT INTO ranks VALUES('tee2', 'CTF', 'ctf2', 1500, 2)");

	/* Ranks for 'tee3' */
	exec("INSERT INTO ranks VALUES('tee3', 'CTF', '', 1400, 3)");
	exec("INSERT INTO ranks VALUES('tee3', 'CTF', 'ctf1', 1400, 3)");
	exec("INSERT INTO ranks VALUES('tee3', 'CTF', 'ctf2', 1400, 3)");

	/* Servers */
	exec("INSERT INTO servers VALUES('1.1.1.1', '8000', 'server1', 'CTF', 'ctf1', time('now'), 0, 16)");

	/* Server masters */
	exec("INSERT INTO server_masters VALUES('1.1.1.1', '8000', 'master2.teerank.com', '9000')");

	/* Server clients */
	exec("INSERT INTO server_clients VALUES('1.1.1.1', '8000', 'tee1', '', 10, 1)");
	exec("INSERT INTO server_clients VALUES('1.1.1.1', '8000', 'tee2', '', 0, 0)");

	return dberr ? 1 : 0;
}
