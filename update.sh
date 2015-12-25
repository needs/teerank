#!/bin/sh

export PATH=.:"$PATH"

#
# Database update
#
init_database
add_new_servers
update_servers | ./update_players
compute_ranks
update_clans
paginate_ranks 100
