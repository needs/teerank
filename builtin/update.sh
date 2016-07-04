#!/bin/sh

set -e

export PATH=.:"$PATH"

teerank-init
teerank-upgrade
teerank-remove-offline-servers 1
teerank-add-new-servers
teerank-update-servers | teerank-update-players | teerank-update-clans
teerank-compute-ranks
