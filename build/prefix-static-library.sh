#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <static_lib> <prefix>" 2>&1
    exit 1
fi

tmpfile=$(mktemp)

# Extract every relevant symbols, cleanup nm output, and append the
# prefixed symbol after each symbol name.  Each line of the file should
# look like this:
#
# 	<symbol_name> <prefixed_symbol_name>
#
nm --defined-only --extern-only "$1" \
    | cut -d' ' -f3 | grep -v -e ":" -e "^$" - \
    | awk -v prefix="$2" '{print $0 " " prefix $0}' >"$tmpfile"

# Use the previously generated file to prefix the desired symbol in the
# given static library
objcopy --redefine-syms="$tmpfile" "$1"

rm "$tmpfile"
