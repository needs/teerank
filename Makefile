TEERANK_VERSION = 2
TEERANK_SUBVERSION = 1
DATABASE_VERSION = 5
STABLE_VERSION = 1

CFLAGS += -lm -Icore -Icgi -Wall -Werror -std=c89 -D_POSIX_C_SOURCE=200809L
CFLAGS += -DTEERANK_VERSION=$(TEERANK_VERSION)
CFLAGS += -DTEERANK_SUBVERSION=$(TEERANK_SUBVERSION)
CFLAGS += -DDATABASE_VERSION=$(DATABASE_VERSION)
CFLAGS += -DSTABLE_VERSION=$(STABLE_VERSION)

BUILTINS_SCRIPTS += upgrade
BUILTINS_SCRIPTS += update
BUILTINS_SCRIPTS := $(addprefix teerank-,$(BUILTINS_SCRIPTS))

UPGRADE_SCRIPTS += upgrade-0-to-1
UPGRADE_SCRIPTS += upgrade-1-to-2
UPGRADE_SCRIPTS += upgrade-2-to-3
UPGRADE_SCRIPTS += upgrade-3-to-4
UPGRADE_SCRIPTS := $(addprefix teerank-,$(UPGRADE_SCRIPTS))

SCRIPTS = $(BUILTINS_SCRIPTS) $(UPGRADE_SCRIPTS)

UPGRADE_BINS += upgrade-4-to-5
UPGRADE_BINS := $(addprefix teerank-,$(UPGRADE_BINS))

# Each builtin have one C file with main() function in "builtin/"
BUILTINS_BINS = $(addprefix teerank-,$(patsubst builtin/%.c,%,$(wildcard builtin/*.c)))

BINS = $(UPGRADE_BINS) $(BUILTINS_BINS)

CGI = teerank.cgi

$(shell mkdir -p generated)

# Add debugging symbols and optimizations to check for more warnings
debug: CFLAGS += -O -g
debug: $(BINS) $(SCRIPTS) $(CGI)

# Remove assertions and enable optimizations
release: CFLAGS += -DNDEBUG -O2
release: $(BINS) $(SCRIPTS) $(CGI)

#
# Binaries
#

# Object files
core_objs = $(patsubst %.c,%.o,$(wildcard core/*.c))
page_objs = $(patsubst %.c,%.o,$(wildcard cgi/*.c) $(wildcard cgi/page/*.c))

# Header file dependancies
core_headers = $(wildcard core/*.h)
page_headers = $(wildcard cgi/*.h)

$(core_objs): $(core_headers)
$(page_objs): $(page_headers) $(core_headers)

# config.c use version constants defined here
$(core_objs): Makefile

$(BINS): $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

$(BUILTINS_BINS): teerank-% : builtin/%.o

teerank-upgrade-4-to-5: $(patsubst %.c,%.o,$(wildcard upgrade/4-to-5/*.c))

#
# Scripts
#

# Script needs to handle configuration variables as well.  The
# necessary code to handle this is generated by a program.  The
# following rule define how to build this program...
build/generate-default-config: build/generate-default-config.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

# This generate a header prepended to every scripts.  This header set
# default values for configuration variables as well as adding the
# current working directory to the PATH.  This is done so that scripts
# will use binaries in the same directory in priority.
#
# Hence doing ./teerank-update for instance will have the desired, as
# it wont use system wide installed binaries (if any).  And doing
# teerank-update in the same directory will use system wide installed
# binaries, as expected too.
generated/script-header.inc.sh: build/generate-default-config
	@echo "#!/bin/sh" >$@
	@echo 'PATH="$$(dirname $$BASH_SOURCE):$$PATH"' >>$@
	./$< >>$@

$(UPGRADE_SCRIPTS): teerank-upgrade-%: upgrade/%.sh
$(BUILTINS_SCRIPTS): teerank-%: builtin/%.sh

$(SCRIPTS): generated/script-header.inc.sh
	cat $^ >$@ && chmod +x $@

#
# CGI
#

$(CGI): cgi/cgi.o cgi/route.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

#
# Lib
#

CURRENT_LIB = libteerank$(DATABASE_VERSION).a
$(CURRENT_LIB): $(core_objs)
	ar cr $@ $^
	objcopy --prefix-symbols=teerank$(DATABASE_VERSION)_ $@

#
# Clean
#

clean:
	rm -f core/*.o builtin/*.o cgi/*.o cgi/page/*.o build/*.o
	rm -f $(BINS) $(SCRIPTS) $(CGI)
	rm -f generated/script-header.inc.sh build/generate-default-config
	rm -r generated/

#
# Install
#

install: TEERANK_ROOT       = $(prefix)/var/lib/teerank
install: TEERANK_DATA_ROOT  = $(prefix)/usr/share/webapps/teerank
install: TEERANK_BIN_ROOT   = $(prefix)/usr/bin
install:
	mkdir -p $(TEERANK_ROOT)
	mkdir -p $(TEERANK_DATA_ROOT)
	mkdir -p $(TEERANK_BIN_ROOT)

	cp $(BINS) $(SCRIPTS) $(TEERANK_BIN_ROOT)
	cp -r $(CGI) assets/* $(TEERANK_DATA_ROOT)

.PHONY: all debug release clean install
