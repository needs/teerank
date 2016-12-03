#!/bin/sh

set -e

function update {
	teerank-add-new-servers
}

while true; do
	update & sleep "$TEERANK_UPDATE_DELAY"m
done &

teerank-update-servers | teerank-update-players
