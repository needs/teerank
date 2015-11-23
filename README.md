Teerank
=======

Teerank is a simple ranking system for teeworlds.  You can test the lastest stable version at [teerank.com](http://teerank.com/) .

How to build and use
====================

To use it, you must compile the binaries first:

```bash
make
```

And then you can generate the index.html file with:

```bash
update.sh
```

A builtin limits of 3 minutes per update is set to have meaningful result.  If you wan't to update continuesly the ranking system, use something like the following:

```bash
while true; do update.sh; sleep 180; done
```
