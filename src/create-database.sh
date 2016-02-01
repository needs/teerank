#!/bin/sh

: ${TEERANK_ROOT:=.teerank}

mkdir -p "$TEERANK_ROOT"
mkdir -p "$TEERANK_ROOT/servers"
mkdir -p "$TEERANK_ROOT/players"
mkdir -p "$TEERANK_ROOT/clans"
mkdir -p "$TEERANK_ROOT/pages"
[ -f "$TEERANK_ROOT/version" ] || echo "$1" > "$TEERANK_ROOT/version"
