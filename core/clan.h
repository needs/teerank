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

	/*
	 * Number of members is the number of members inside the array,
	 * while max_nmembers is the maximum number of player the array
	 * can hold, which _should_ be greater or equal to nmembers.
	 */
	unsigned nmembers, max_nmembers;
	struct player_info *members;
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
 * @return SUCCESS on success, NOT_FOUND when does not exist, FAILURE
 *         otherwise.
 */
int read_clan(struct clan *clan, const char *name);

/**
 * Create and initialize a new clan
 *
 * No entry in the database are created yet: write_clan() should be
 * called to save clan in the database.
 *
 * nmembers Should be higher than zero, it doesn't make sen to create an
 * empty clan.
 *
 * @param clan Clan to be created
 * @param name Name of the new clan
 * @param nmembers Number of members in the clan
 */
void create_clan(struct clan *clan, const char *name, unsigned nmembers);

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
struct player_info *get_member(struct clan *clan, char *name);

/**
 * Add the given member to the member list and return it.
 *
 * @param clan Clan to add member in
 * @param name Name of the new member
 *
 * @return A valid pointer to the new member on success, NULL otherwise
 */
struct player_info *add_member(struct clan *clan, char *name);

/**
 * Remove the given member from the clan.
 *
 * If the given member does not belongs to the given clan, behavior is
 * undefined.
 *
 * @param clan Actual clan of the member to be removed
 * @param member Valid pointer to the member to be removed
 */
void remove_member(struct clan *clan, struct player_info *member);

/**
 * Fill in members data
 *
 * Load player's using read_player_info().  If a read fail, the player is
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
 * Remove the given clan
 *
 * @param cname Name of the clan to remove
 *
 * @return 1 if clan has been successfully removed, 0 otherwise
 */
int remove_clan(const char *cname);

#endif /* CLAN_H */
