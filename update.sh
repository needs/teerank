#!/bin/sh

db=data

mkdir -p $db/servers $db/players $db/clans

#
# Database update
#

# Update servers list and players
./add_new_servers $db/servers
./update_servers $db/servers/* | ./update_players $db/players
./compute_ranks $db/players
./update_clans $db/clans $db/players

#
# HTML pages generation
#

# Index
./generate_index $db/players/ > index.html

# Rank pages
mkdir -p pages
./generate_rank_pages $db/players pages/

# Clan pages
mkdir -p clans
for i in data/clans/*; do
    ./generate_clan_page "$i" data/players/ >"clans/$(basename $i).html"
done
