#!/bin/sh

export PATH=.:"$PATH"

#
# Database update
#
teerank-init-database
teerank-add-new-servers
teerank-update-servers | teerank-update-players
teerank-compute-ranks
teerank-update-clans
teerank-paginate-ranks 100
