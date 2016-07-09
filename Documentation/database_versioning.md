Database versioning
===================

Data layout in database follow strict versioning rules.  This rules aim
to provide data continuity over versions and making sure data can be
ported to new teerank releases.

You can find database version number in "$TEERANK_ROOT/version".  This
number can be different than teerank version, because in the past
database version and teerank version were decoupled.

Each new teerank release can (it is not mandatory) increase database
version number.  If so, an upgrade script should be provided to migrate
data from the previous version to the new one.

Teerank snapshots between two teerank releases - a checkout on a
particular commit for instance - only does provide a script to migrate
data from the previous release to the current snapshot.

Hence database upgraded to one particular snapshot may not be upgradable
to the next teerank release.  Hence you should *NEVER* use developpement
snapshots for a production use of teerank.

See variables on the top of the main Makefile to query the current
teerank version, database version, and wheter or not this is a stable
snapshot.  Stable snapshots are guaranteed to keep data integrity and
newer release of teerank will provide a script to upgrade database.

Upgrading
=========

Use teerank-upgrade to upgrade your database.  A prompt and a warning
message will be displayed if you are upgrading to an unstable database
layout.

Before upgrading, you SHOULD make a backup of your data.  If for some
reason the upgrading process fail, it will likely leave the database in
an unknown state, that may or may not work when you run teerank-upgrade
again.
