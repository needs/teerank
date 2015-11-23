Teerank
=======

Teerank is a simple ranking system for teeworlds.  You can test the lastest stable version at [teerank.com](http://teerank.com/) .

How to build and use
====================

To use it, you must first compile the binaries.

```bash
make
```

And then you can generate the `index.html` file with:

```bash
update.sh
```

A builtin limit of 3 minutes per update is set in order to have meaningful results.  If you want to update continuously the ranking system, use something like the following:

```bash
while true; do update.sh; sleep 180; done
```
