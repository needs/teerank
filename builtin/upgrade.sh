#!/bin/sh

# Since teerank_root is not a thing anymore, we have to redefine it here
# for the old database
if [ -z "$TEERANK_ROOT" ]; then TEERANK_ROOT=.teerank; fi

file="$TEERANK_ROOT/version"

if [ -f "$file" ]; then
	version=$(< "$file")
else
	version=DATABASE_VERSION
fi

if (( version == $DATABASE_VERSION )); then
	echo "Database already upgraded to the latest version ($version)"
	exit 0
elif (( version + 1 != $DATABASE_VERSION )); then
	echo "Database too old to be upgraded ($version), use an older"
	echo" version of teerank to upgrade it"
	exit 0
fi

if [ $version -lt $DATABASE_VERSION ] && [ $STABLE_VERSION -eq 0 ]; then
	echo "Warning: data are being upgraded to an unstable database format."
	echo "         This format may not be portable to newer stable release"
	echo -n "Type \"continue\" to proceed: "
	read answer

	if [ $answer != "continue" ]; then
		echo "Upgrade canceled"
		exit 1
	fi

	echo
fi

echo "Upgrading from $version to $((version + 1))..."

teerank-upgrade-$version-to-$(($version + 1)) || {
	echo "Error while upgrading from $version to $(($version + 1))"
	exit 1
}

echo "Success!"
