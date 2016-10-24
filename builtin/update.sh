#!/bin/sh

set -e

# Create database and check config
teerank-init

#
# Trap signals because stopping at an unexpected point might corrupt the
# database at worst.
#
function update {
	trap "" INT TERM QUIT
	teerank-add-new-servers
	teerank-remove-offline-servers 1
	teerank-update-servers | teerank-update-players | teerank-update-clans
	teerank-build-indexes
	trap - INT TERM QUIT
}

while true; do update & sleep "$TEERANK_UPDATE_DELAY"m; done
