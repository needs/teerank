#!/bin/sh

file="$TEERANK_ROOT/version"

if [ -f "$file" ]; then
	version=$(< "$file")
else
	version=0
fi

if (( version == $DATABASE_VERSION )); then
	echo "Database already upgraded to the latest version ($version)"
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

backup="teerank.$version.upgrade-backup.tar.gz"

echo "Creating backup \"$backup\""
tar czf "$backup" "$TEERANK_ROOT" || {
	echo "Cannot create backup, aborting"
	exit 1
}

while (( version < $DATABASE_VERSION )); do
        echo "Upgrading from $version to $((version + 1))..."

	teerank-upgrade-$version-to-$(($version + 1)) || {
		echo "Error while upgrading from $version to $(($version + 1))"
		echo "Restoring database with backup \"$backup\""

		# We could use rm -r but I'm actually afraid of any
		# mistake that will delete the wrong directory
		tar xzf "$backup"
		rm "$backup"

		exit 1
	}

	((version++))
	echo "$version" > "$file"
done

echo "Success! Removing backup"
rm "$backup"
