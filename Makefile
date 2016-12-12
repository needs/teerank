TEERANK_VERSION = 4
TEERANK_SUBVERSION = 0
DATABASE_VERSION = 7
STABLE_VERSION = 1

# Used to make a direct URL to github
CURRENT_COMMIT = $(shell git rev-parse HEAD)
CURRENT_BRANCH = $(shell git rev-parse --abbrev-ref HEAD)

# Thoses are used later to build the previous teerank version
PREVIOUS_VERSION = $(shell expr $(TEERANK_VERSION) - 1)
PREVIOUS_DATABASE_VERSION = $(shell expr $(DATABASE_VERSION) - 1)

# This can be set to zero in the command line by the user to remove the
# support of old URLs.  By default we support them to make sure we don't
# accidentaly break existing teerank installations.

ROUTE_V2_URLS ?= 1
ROUTE_V3_URLS ?= 1

# We want to be C89 to make sure teerank can be built using almost any
# compilers on almost any machines you can think of.
#
# Also warnings (treated as error) are mandatory to catch bugs and
# misuses of the C language.

CFLAGS += \
	-std=c89 \
	-Wall -Werror \
	-Icore -Icgi \
	-lm \
	-lsqlite3

# Using POSIX 2008 is another safeguard against unwanted lib C
# extensions.  We could use an older POSIX standard but it turns out
# useful functions are only available in POSIX 2008.

CFLAGS += -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700

# Sends every build infos to the C preprocessor

CFLAGS += \
	-DTEERANK_VERSION=$(TEERANK_VERSION) \
	-DTEERANK_SUBVERSION=$(TEERANK_SUBVERSION) \
	-DDATABASE_VERSION=$(DATABASE_VERSION) \
	-DSTABLE_VERSION=$(STABLE_VERSION) \
	-DCURRENT_COMMIT=\"$(CURRENT_COMMIT)\" \
	-DCURRENT_BRANCH=\"$(CURRENT_BRANCH)\" \
	-DROUTE_V2_URLS=$(ROUTE_V2_URLS) \
	-DROUTE_V3_URLS=$(ROUTE_V3_URLS)

UPDATE_BIN = teerank-update
UPGRADE_BIN = teerank-upgrade
CGI = teerank.cgi

BINS = $(UPGRADE_BIN) $(UPDATE_BIN) $(CGI)

$(shell mkdir -p generated)

# Add debugging symbols and optimizations to check for more warnings
debug: CFLAGS += -O -g
debug: CFLAGS_EXTRA = -O -g
debug: $(BINS) $(CGI)

# Remove assertions and enable optimizations
release: CFLAGS += -DNDEBUG -O2
release: CFLAGS_EXTRA = -DNDEBUG -O2
release: $(BINS) $(CGI)

# Object files
core_objs    = $(patsubst %.c,%.o,$(wildcard core/*.c))
cgi_objs     = $(patsubst %.c,%.o,$(wildcard cgi/*.c) $(wildcard cgi/page/*.c))
update_objs  = $(patsubst %.c,%.o,$(wildcard update/*.c))
upgrade_objs = $(patsubst %.c,%.o,$(wildcard upgrade/*.c))

# Header files
core_headers    = $(wildcard core/*.h)
update_headers  = $(wildcard update/*.h)
upgrade_headers = $(wildcard upgrade/*.h)
cgi_headers     = $(wildcard cgi/*.h)

# Header files dependencies
$(core_objs):    $(core_headers)
$(update_objs):  $(core_headers) $(update_headers)
$(upgrade_objs): $(core_headers) $(upgrade_headers)
$(cgi_objs):     $(core_headers) $(cgi_headers)

# Binaries objects dependencies
$(UPDATE_BIN):  $(core_objs) $(update_objs)
$(UPGRADE_BIN): $(core_objs) $(upgrade_objs)
$(CGI):         $(core_objs) $(cgi_objs)

$(BINS):
	$(CC) $(CFLAGS) -o $@ $^

# The upgrade binary need in order to be built a static library of the
# *previous* teerank version.  In order to get it, we will extract the
# previous teerank version from the git historic.  Built it, and
# finally, make sure exported symbols in core header files are prefixed
# to avoid name clashes with our current code.

PREVIOUS_LIB = libteerank$(PREVIOUS_DATABASE_VERSION).a
PREVIOUS_BRANCH = teerank-$(PREVIOUS_VERSION).y

# Header files needs to be prefixed
build/prefix-header: build/prefix-header.o
	$(CC) -o $@ $(CFLAGS) $^

$(PREVIOUS_LIB): dest = .build/teerank$(PREVIOUS_DATABASE_VERSION)
$(PREVIOUS_LIB): .git/refs/heads/$(PREVIOUS_BRANCH) build/prefix-header
	mkdir -p $(dest)
	git archive $(PREVIOUS_BRANCH) | tar x -C $(dest)
	CFLAGS="$(CFLAGS_EXTRA)" $(MAKE) -C $(dest) $(PREVIOUS_LIB)
	cp $(dest)/$(PREVIOUS_LIB) .

	# Prefix every header file in core
	for i in $(dest)/core/*.h; do \
		./build/prefix-header teerank$(PREVIOUS_DATABASE_VERSION)_ <"$$i" >"$$i".tmp; \
		mv "$$i".tmp "$$i"; \
	done

# Upgrade binary just need the static library of the previous teerank
# version as well as a path the the previous header files as well.

$(upgrade_objs) $(UPGRADE_BIN): CFLAGS += -I.build
$(upgrade_objs): $(PREVIOUS_LIB)
$(UPGRADE_BIN): $(PREVIOUS_LIB)


#
# Clean
#

clean:
	rm -f core/*.o update/*.o upgrade/*.o cgi/*.o cgi/page/*.o build/*.o
	rm -f $(BINS)
	rm -f $(PREVIOUS_LIB)
	rm -f build/prefix-header
	rm -rf .build/

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
