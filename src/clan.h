#ifndef CLAN_H
#define CLAN_H

#include "player.h"

struct clan {
	char name[MAX_CLAN_HEX_LENGTH];

	unsigned length;
	struct player *members;
};

/*
 * Read the given clan entry.  Player's struct are not fully loaded as only
 * the name is set.
 *
 * Return 1 on success and 0 on failure.
 */
int read_clan(struct clan *clan, char *name);

/*
 * Write the given clan on the disk.  Members do not need to be loaded.
 *
 * Return 1 on success and 0 on failure.
 */
int write_clan(const struct clan *clan);

/*
 * Free memory holded by a clan struct and reset it.
 */
void free_clan(struct clan *clan);

/*
 * Search for a member with the given name.
 *
 * Return NULL if not found.
 */
struct player *get_member(struct clan *clan, char *name);

/*
 * Add the given member to the member list and return it.
 *
 * Return NULL on failure.
 */
struct player *add_member(struct clan *clan, char *name);

/*
 * Remove the given member from the members list.
 */
void remove_member(struct clan *clan, struct player *member);

/*
 * Load player's struct using read_player().  If a read fail, the player is
 * removed from the members list.
 *
 * Return the number of removed members.
 */
unsigned load_members(struct clan *clan);

/*
 * Compare two clans, return 1 if they are exactly the same.
 *
 * Two clan are equals when they have the same name, the same number of
 * members and each members are the same.
 */
int clan_equal(const struct clan *c1, const struct clan *c2);

#endif /* CLAN_H */
