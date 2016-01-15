Teerank
=======

Teerank is a simple and fast ranking system for teeworlds.  You can
test the lastest stable version at [teerank.com](http://teerank.com/).

How to build
============

```bash
make
```

How to use
==========

Teerank generate HTML files from a database.  So in order to get the
HTML files, you must create and fill a database.  This is done by
running:

```bash
./teerank-update
```

It put the database in `./.teerank` by default, check it out!
The, use teerank.cgi to generate some HTML pages:

```bash
PATH=. DOCUMENT_URI=/pages/1.html ./teerank.cgi
```

In order to have the CSS, you have to start a webserver at the root of your
git repository.  Here is a simple way of doing it:

```bash
python -m http.server &
```

You can then see the previously generated page in your browser:

```bash
firefox pages/1.html
```

Advanced use
============

To update continuesly the database, use something like:

```bash
while true; do ./teerank-update; sleep 300; done
```

You may want to use teerank.cgi as a CGI.  That way, pages will be
generated on demand, and then cached for further reuse if the database
hasn't changed.

First, install teerank on your system using `make install`.  Then
configure you webserver.  Here is an example for nginx, with FastCGI.

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
			fastcgi_param TEERANK_CACHE_ROOT /var/cache/teerank;

			fastcgi_param SCRIPT_FILENAME $document_root/teerank.cgi;
			fastcgi_pass  unix:/run/fcgiwrap.sock;
		}
	}
}
```

The environement variables `$TEERANK_ROOT` must point to the database,
and `$TEERANK_CACHE_ROOT` the were cached files will be stored.  Be
sure to run `teerank-update` with the appropriate values:

```bash
TEERANK_ROOT=/var/lib/teerank teerank-update
```

You may have to change permission to get it working.

Tips
====

On archlinux, you can use pacman to actually install teerank
system-wide:

```bash
makepkg -i BUILDDIR=/tmp/makepkg
```

Contributing
============

Teerank is a free software under the GNU GPL v3.

All the development take place on github.  Feel free to open issue on
github or send pull-request.  If you don't want to use github you can
also send me an e-mail at needs@mailoo.org.
