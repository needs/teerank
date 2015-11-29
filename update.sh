#!/bin/sh

db=data

mkdir -p $db/servers $db/players $db/clans

# Update servers list and players
./add_new_servers $db/servers
./update_servers $db/servers/* | ./update_players $db/players
./compute_ranks $db/players
./update_clans $db/clans $db/players

# Generate HTML page
./generate_index $db/players/ > index.html

# Clan pages
mkdir -p clans
for i in data/clans/*; do
    ./generate_clan_page "$i" data/players/ >"clans/$(basename $i).html"
done
