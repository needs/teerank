#!/bin/sh

file="$TEERANK_ROOT/version"

if [ -f "$file" ]; then
	version=$(< "$file")
else
	version=0
fi

while command -v teerank-upgrade-$version-to-$(($version + 1)) >/dev/null; do
	teerank-upgrade-$version-to-$(($version + 1))
	((version++))
	echo "$version" > "$file"
done
