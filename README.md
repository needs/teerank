Teerank
=======

Teerank is a simple ranking system for teeworlds.  You can test the lastest stable version at [teerank.com](http://teerank.com/).

How to build
============

```bash
make
```

How to use
==========

Teerank use a database to generate HTML files.  In order to get the
HTML files, you must create and fill the database.  This is done by
running:

```bash
./teerank-update
```

If you want to update continuously the ranking system, use something like the following:

```bash
while true; do ./teerank-update; sleep 300; done
```

Now you have to generate HTML file.  For that configure your web
server to use teerank.cgi as a CGI.  Here is an example with nginx
that use the cloned repository as a root:

```
http {
	server {
		listen       8000;
		server_name  teerank.com;
		root         <path_to_cloned_repo>;

		rewrite ^/$ /pages/1.html redirect;
		try_files $uri @teerank;
		location @teerank {
			include       fastcgi_params;
			fastcgi_param SCRIPT_FILENAME $document_root/teerank.cgi;
			fastcgi_pass  unix:/run/fcgiwrap.sock;
		}
	}
}
```

How to install
==============

You can use the traditional way:

```bash
sudo make install
```

Or install it as a package, so you can easily remove it:

```bash
# Archlinux
makepkg -i BUILDDIR=/tmp/makepkg
```

Generate the database using:

```bash
TEERANK_ROOT=/var/lib/teerank teerank-update
```

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

Contributing
============

Teerank is a free software under the GNU GPL v3.

All the development take place on github.  Feel free to open issue on
github or send pull-request.  If you don't want to use github you can
also send me an e-mail at needs@mailoo.org.
