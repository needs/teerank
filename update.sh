#!/bin/sh

db=data

mkdir -p $db/servers $db/players

# Update servers list
./add_new_servers $db/servers
./update_servers $db/servers/* | ./update_players_infos $db/players | ./update_players_elo $db/players

# Generate HTML page
./generate_index $db/players/ > index.html
