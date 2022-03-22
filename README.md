# Teerank.io

Teerank is a simple and fast ranking system for Teeworlds. You can test the
lastest stable version at teerank.io.

# Run

```bash
# Production
$ docker-compose up --build

# Run tests loop
$ docker-compose -f docker-compose-test.yml -p teerank-dev up --build

# Development server at localhost:80
$ docker-compose -f docker-compose-development.yml -p teerank-test up --build
```

I usually launch tests and the development server on two different shells.
When a source file is changed, tests will be run again and the development
server will be relaunch with the new version.  Hence once the containers are
launched there is no need to run them again: just refresh your page in the
browser and check test logs and that's it.

# Legacy

This project is a reboot of teerank.com.  The former was written in C and used
SQLite.  While good, this technologies are arguably not the best tools in the
box for making a ranking website.

This project instead relies on Python, Redis and Flask.  Performance is not
really a concern so Python is a good language of choice, as it improves
developement speed and is accessible to more people.  Redis is handy because
it provides a way to sort our players directly while SQLite doesn't.

Finally, the old teerank frontend was a C program compiled into a CGI which
then was plugged into Nginx.  It worked, but it was difficult to setup.  We
choose to use Flask, because it fit our needs and Flask deals with all the
nasty parts under the hood.

Also the domain teerank.com expired by accident, and was immediately
cyber-squatted.  They refused my buying offers, so at some point I stopped
the server and let the project die.  Now the domain is available again but
due to the premium system for .com demains, it costs more than a thousant
dollars, which is not something I want ot pay for a pet project.  That's why
teerank is now available at teerank.io.

# Goals

  - Reference every servers and players accross all teeworlds versions;
  - Provide a ranking system for every mods;
  - Gather historical data on players, servers, maps, clans and more;
