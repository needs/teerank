#!/bin/sh

for i in "$TEERANK_ROOT"/players/*; do
	if [ -f "$i/clan" ] && [ -f "$i/elo" ] && [ -f "$i/rank" ]; then
		echo "$(cat $i/clan) $(cat $i/elo) $(cat $i/rank)" >"$i/infos"
		rm "$i/clan" "$i/elo" "$i/rank"
	fi
done
