#ifndef CLAN_H
#define CLAN_H

#include "player.h"

/**
 * @struct clan
 *
 * Basically a list of players.
 */
struct clan {
	char name[HEXNAME_LENGTH];

	unsigned length;
	struct player_summary *members;
};

/**
 * Read the given clan.
 *
 * Player's struct are not fully loaded as only the name is set.  To
 * load members, use load_members().
 *
 * @param clan Clan structure to contain readed data
 * @param name Clan name
 *
 * @return 1 on success, 0 on failure
 */
int read_clan(struct clan *clan, const char *name);

/**
 * Write the given clan on the disk.
 *
 * Members do not need to be loaded.
 *
 * @param clan Clan structure to be written
 *
 * @return 1 on success, 0 on failure
 */
int write_clan(const struct clan *clan);

/**
 * Free memory holded by a clan struct and reset it.
 *
 * @param clan Clan structure to free
 */
void free_clan(struct clan *clan);

/**
 * Search for a member with the given name.
 *
 * @param clan Clan to search for member
 * @param name Member name
 *
 * @return A valid pointer to a member if found, NULL otherwise
 */
struct player_summary *get_member(struct clan *clan, char *name);

/**
 * Add the given member to the member list and return it.
 *
 * @param clan Clan to add member in
 * @param name Name of the new member
 *
 * @return A valid pointer to the new member on success, NULL otherwise
 */
struct player_summary *add_member(struct clan *clan, char *name);

/**
 * Remove the given member from the clan.
 *
 * If the given member does not belongs to the given clan, behavior is
 * undefined.
 *
 * @param clan Actual clan of the member to be removed
 * @param member Valid pointer to the member to be removed
 */
void remove_member(struct clan *clan, struct player_summary *member);

/**
 * Fill in members data
 *
 * Load player's using read_player_summary().  If a read fail, the player is
 * removed from the members list.
 *
 * @param clan Clan to load members
 *
 * @return Number of removed members
 */
unsigned load_members(struct clan *clan);

/**
 * Compare two clans
 *
 * Two clan are equals when they have the same name, the same number of
 * members and each members are the same.
 *
 * @param c1 Clan to compare
 * @param c2 Clan to compare against the first one
 *
 * @return 1 if clans are the same, 0 otherwise
 */
int clan_equal(const struct clan *c1, const struct clan *c2);

/**
 * Add a member to the given clan without loading the full memberlist.
 *
 * Reading a clan using read_clan() does require a malloc() for the
 * member list.  In the case of adding a new player, the process can
 * be faster by just opening the file in "a" mode and appending the
 * new player name.
 *
 * @param clan Clan filename
 * @param player Name of the new player
 *
 * @return 1 on success, 0 on failure
 */
int add_member_inline(char *clan, char *player);

#endif /* CLAN_H */
