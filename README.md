Teerank
=======

Teerank is a simple and fast ranking system for Teeworlds.  You can
test the lastest stable version at [teerank.com](http://teerank.com/).

How to use
==========

Build it:

```bash
make
```

Then update database at least once:

```
./teerank-update
```

Finally configure your webserver.  Root path must point to `assets`, so
that `/style.css` can be resolved.  Database and CGI are located in the
parent directory by default, hence `$document_root/../` to access them.
Here is an example for Nginx:

```
http {
	server {
		listen       8000;
		server_name  teerank.com;
		root         /home/foo/teerank/assets;

		try_files $uri @teerank;
		location @teerank {
			include       fastcgi_params;

			fastcgi_param TEERANK_DB      $document_root/../teerank.sqlite3;
			fastcgi_param SCRIPT_FILENAME $document_root/../teerank.cgi;
			fastcgi_pass  unix:/run/fcgiwrap.sock;
		}
	}
}
```

The CGI require write persmissions on the database file, as well as any
files created by sqlite itself.  This is because we are using
[WAL](https://www.sqlite.org/wal.html) for performances reasons.

Once everything is set up, check `localhost:8000` or whatever location you
choosed and you should be good to go.

Advanced use
============

You may want to build a version opimized for your own server using:

```bash
make -B release
```

Also, you can specify additional `CFLAGS` and change compiler, to build
statically linked binaries for instance:

```bash
CC="musl-gcc" CFLAGS="-static" make -B release
```

When running `teerank-update`, set `TEERANK_DB` to change database
location, and `TEERANK_VERBOSE` to `1` to enable verbose mode.

Setting up a CGI for developpement may be cumbursome, you can actually
simulate CGI environment with the command line, like so:

```bash
REQUEST_URI="/search?q=Nameless" ./teerank.cgi
```

Upgrading from a previous version
=================================

Database from the previous stable version can be upgraded using:

```
./teerank-upgrade
```

Keep in mind that upgrades are only supported between stable version of
teerank.  It means that database created or upgraded while being on an
unstable version will likely not be upgradable to the next stable teerank.

Contributing
============

Teerank is a free software under the GNU GPL v3.

All the development takes place on github.  Feel free to open issue on
github or send pull-requests.  If you don't want to use github you can
also send me an e-mail at needs@mailoo.org.
