Teerank
=======

Teerank is a simple and fast ranking system for teeworlds.  You can
test the lastest stable version at [teerank.com](http://teerank.com/).

How to build
============

```bash
make
```

You may want to build a version opimized for your own server using:

```bash
make -B release
```

Also, you can specify additional `CFLAGS` and change compiler, to build
statically linked binaries for instance:

```bash
CC="musl-gcc" CFLAGS="-static" make -B release
```

How to use
==========

Teerank is split in two parts: a set of program to populate a database,
and a CGI as a user interface for the database.

To populate the database, use `teerank-update`.

```bash
./teerank-update
```

Call it within regular intervals so that you can rank players over time:

```bash
while true; do ./teerank-update & sleep 300; done
```

By default, database is generated in `.teerank`, you can overide this
setting using `$TEERANK_ROOT`.  You can also enable verbose mode which
should give more insight on what's going on.

```bash
TEERANK_ROOT=/var/lib/teerank TEERANK_VERBOSE=1 ./teerank-update
```

Setting up CGI
==============

First off, it is recommended to install teerank system-wide using:

```bash
make install
```

Then configure your webserver to use installed cgi (by default in
`/usr/share/webapps/teerank`).  here is an example for Nginx, assuming
database is located at `/var/lib/teerank`.

```
http {
	server {
		listen       80;
		server_name  teerank.com;
		root         /usr/share/webapps/teerank;

		rewrite ^/$ /pages/1.html redirect;
		try_files $uri @teerank;
		location @teerank {
			include       fastcgi_params;

			fastcgi_param TEERANK_ROOT       /var/lib/teerank;

			fastcgi_param SCRIPT_FILENAME $document_root/teerank.cgi;
			fastcgi_pass  unix:/run/fcgiwrap.sock;
		}
	}
}
```

Tips
====

On archlinux, you can use pacman to actually install teerank
system-wide:

```bash
makepkg -i BUILDDIR=/tmp/makepkg
```

Setting up a CGI for developpement may be cumbursome, you can actually
simulate CGI environement with the command line, like so:

```bash
DOCUMENT_URI="/search" QUERY_STRING="q=Nameless" ./teerank.cgi >assets/test.html
```

Launch a lightweight server in `assets/` to then access the generated
page at [localhost:8000/test.html](http://localhost:8000/test.html):

```bash
cd assets && python -m http.server &
```

Previous teerank version used different urls layout, by default they
route to the new ones.  If you don't want to support them at all, build
with `DONT_ROUTE_OLD_URLS` defined.

```bash
CFLAGS="-DDONT_ROUTE_OLD_URLS" make -B
```

Contributing
============

Teerank is a free software under the GNU GPL v3.

All the development take place on github.  Feel free to open issue on
github or send pull-request.  If you don't want to use github you can
also send me an e-mail at needs@mailoo.org.
