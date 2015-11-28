Teerank
=======

Teerank is a simple ranking system for teeworlds.  You can test the lastest stable version at [teerank.com](http://teerank.com/).

How to build and use
====================

To use it, you must first compile the binaries.

```bash
make
```

Then you can generate the `index.html` file with:

```bash
./update.sh
```

If you want to update continuously the ranking system, use something like the following:

```bash
while true; do ./update.sh; sleep 180; done
```

To have images and CSS you need to start an HTTP server in this directory.  Here is a simple way to do it for debugging purpose:
```bash
python -m http.server
```

Contributing
============

Send pull request using github interface or send me an email (needs@mailoo.org) with a pull request or an attached patch.  Feel free to open issue on github and comment them.
