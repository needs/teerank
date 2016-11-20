#!/bin/sh

set -e

function update {
	teerank-add-new-servers
	teerank-remove-offline-servers 1
	teerank-update-servers | teerank-update-players
}

while true; do
	update & sleep "$TEERANK_UPDATE_DELAY"m
done
