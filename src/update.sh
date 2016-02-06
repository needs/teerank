#!/bin/sh

version=1
export PATH=.:"$PATH"

#
# Database update
#
teerank-create-database $version
teerank-upgrade $version
teerank-remove-offline-servers 1
teerank-add-new-servers
teerank-update-servers | teerank-update-players
teerank-compute-ranks
teerank-update-clans
teerank-paginate-ranks 100
