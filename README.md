# Teerank.io

Teerank is a simple and fast ranking system for Teeworlds. You can test the
lastest stable version at teerank.io.

# Build

```bash
docker build -t teerank
```

# Run

```bash
docker run teerank
```

# Legacy

This project is a reboot of teerank.com.  The former was written in C and used
SQLite.  While good, this technologies are arguably not the best tools in the
box for making a ranking website.

This project instead use Python, since performances is not a concern.  We also
replaced SQLite by Redis, because redis offers a way to sort players directly
with no effort, SQLite does not.

Finally, the old teerank frontend was a C program compiled into a CGI which
then was plugged into Nginx.  It worked, but it was difficult to setup.  We
choose to use Flask, because it fit our needs and Flask deals with all the
nasty parts under the hood.

Also the domain teerank.com expired by accident, and was immediately
cyber-squatted.  They refused my buying offers, so at some point I stopped
the server and let the project die.  Now the domain is available again but
due to the premium system, it costs more than a thousant dollars, which is
not something I want ot pay for a pet project.  That's why teerank is now
available at teerank.io.

# Goals

  - Reference every servers and players accross all teeworlds versions;
  - Provide a ranking system for every kind of mods;
  - Gather historical data on players, servers, maps, clans and more;
