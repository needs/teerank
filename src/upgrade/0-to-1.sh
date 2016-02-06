#!/bin/sh

: ${TEERANK_ROOT:=.teerank}
for i in "$TEERANK_ROOT"/servers/*; do
	mv "$i/infos" "$i/state" 2>/dev/null
done
