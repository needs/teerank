#!/bin/sh

for i in "$TEERANK_ROOT"/servers/*; do
	mv "$i/infos" "$i/state" 2>/dev/null
done
