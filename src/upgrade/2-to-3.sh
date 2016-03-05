#!/bin/sh

for i in "$TEERANK_ROOT"/players/*; do
	if [ -f "$i/infos" ]; then
		tmp="$TEERANK_ROOT/players/tmp-infos"
		mv "$i/infos" "$tmp"
		rmdir "$i"
		mv "$tmp" "$i"
	fi
done
