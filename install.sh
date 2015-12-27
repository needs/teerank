#!/bin/sh

# Generic (hopefully) POSIX installation script

TEERANK_ROOT=$1/var/lib/teerank
TEERANK_CACHE_ROOT=$1/var/cache/teerank
TEERANK_DATA_ROOT=$1/usr/share/webapps/teerank
TEERANK_BIN_ROOT=$1/usr/bin

mkdir -p "$TEERANK_ROOT"
mkdir -p "$TEERANK_CACHE_ROOT"
mkdir -p "$TEERANK_DATA_ROOT"
mkdir -p "$TEERANK_BIN_ROOT"

cp teerank-* $TEERANK_BIN_ROOT
cp -r teerank.cgi style.css images $TEERANK_DATA_ROOT
