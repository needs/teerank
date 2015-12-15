#!/bin/sh

db=data

mkdir -p $db/servers $db/players $db/clans $db/pages

#
# Database update
#

# Update servers list and players
./add_new_servers $db/servers
./update_servers $db/servers/* | ./update_players $db/players
./compute_ranks $db/players >$db/ranks
./update_clans $db/clans $db/players
./paginate_ranks <$db/ranks 100 $db/pages/%u

#
# HTML pages generation
#

# Index
./generate_index $db/players/ >index.html

# Rank pages
mkdir -p pages
for file in $db/pages/*; do
    ./generate_rank_page full-page $db/players <$file >pages/$(basename $file).html
    ./generate_rank_page only-rows $db/players <$file >pages/$(basename $file).inc.html
done

# Clan pages
mkdir -p clans
for i in data/clans/*; do
    ./generate_clan_page "$i" data/players/ >"clans/$(basename $i).html"
done
