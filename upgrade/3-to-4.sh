#!/bin/sh

for i in "$TEERANK_ROOT"/clans/*; do
	if [ -f "$i/members" ]; then
		tmp="$TEERANK_ROOT/clans/tmp-members"
		mv "$i/members" "$tmp"
		rmdir "$i"
		mv "$tmp" "$i"
	fi
done
