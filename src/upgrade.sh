#!/bin/bash

: ${TEERANK_ROOT:=.teerank}
export TEERANK_ROOT
file="$TEERANK_ROOT/version"

if [ $# -ne 1 ]; then
	echo "usage: $0 version" 2>&1
	exit
fi

if [ -f "$file" ]; then
	version=$(< "$file")
else
	version=0
fi

for (( version ; version < "$1" ; version++ )); do
	teerank-upgrade-$version-to-$(($version + 1))
done

echo "$version" > "$file"
