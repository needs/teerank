#!/bin/sh

file="$TEERANK_ROOT/version"

if [ -f "$file" ]; then
	version=$(< "$file")
else
	version=0
fi

if [ $version -lt $DATABASE_VERSION ] && [ $STABLE_VERSION -eq 0 ]; then
	echo "Warning: data are being upgraded to an unstable database format" 1>&2
	echo "         this format may not be portable to newer stable release" 1>&2
	echo -n "Type \"continue\" to proceed: " 1>&2
	read answer

	if [ $answer != "continue" ]; then
		echo "Upgrade canceled" 1>&2
		exit 1
	fi
fi

while (( version < $DATABASE_VERSION )); do
	if [ $TEERANK_VERBOSE -eq 1 ]; then
        	echo "Upgrading from $version to $((version + 1))" 1>&2
	fi

	teerank-upgrade-$version-to-$(($version + 1)) || exit 1

	((version++))
	echo "$version" > "$file"
done
