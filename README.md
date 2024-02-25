# [Teerank.io](https://teerank.io/)

[Teerank.io](https://teerank.io/) is a simple and fast ranking system for
Teeworlds. You can test the lastest stable version at teerank.io.

# Build and run

```
npm install
docker compose up -d
npx nx run frontend:serve
npx nx run worker:serve
```

Alternatively, open the projetc on VSCode and run the start building using:
`Command + Shift + B`.

# Legacy

This project is a reboot of teerank.com.  The former was written in C and used
SQLite.  While good, this technologies are arguably not the best tools in the
box for making and maintening a ranking system.

This project instead relies on Typescript, NextJS, PostgreSQL. Performance is
not really a concern so Typescript is a good language to use, as it improves
developement speed and is accessible to more people, it also make concurrency
easy.

Finally, the old teerank frontend was a C program compiled into a CGI which then
was plugged into Nginx.  It worked, but it was difficult to setup.  Using NextJS
saves a lot of time and is more flexible.  Frontend, worker and database are all
hosted on fly.io.

Also the domain teerank.com expired by accident, and was immediately
cyber-squatted.  They refused my buying offers, so at some point I stopped the
server and let the project die.  Now the domain is available again but due to
the premium system for .com demains, it costs more than a thousand dollars,
which is not something I can afford for a pet project.  That's why teerank is
now available at [teerank.io](https://teerank.io/).

# Goals

  - Reference every servers and players accross all teeworlds versions;
  - Provide a ranking system for every mods;
  - Gather historical data on players, servers, maps, clans and more;
