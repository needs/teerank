TEERANK_VERSION = 5
TEERANK_SUBVERSION = 0
DATABASE_VERSION = 8
STABLE_VERSION = 0

# Used to make a direct URL to github
CURRENT_COMMIT = $(shell git rev-parse HEAD)
CURRENT_BRANCH = $(shell git rev-parse --abbrev-ref HEAD)

# We want to be C89 to make sure teerank can be built using almost any
# compilers on almost any machines you can think of.
#
# Also warnings (treated as error) are mandatory to catch bugs and
# misuses of the C language.

CFLAGS += \
	-std=c89 \
	-Wall -Werror \
	-Icore -Icgi

LDFLAGS += \
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

UPDATE_BIN = teerank-update
UPGRADE_BIN = teerank-upgrade
CGI = teerank.cgi
TEST_BIN = teerank-create-test-database

BINS = $(UPGRADE_BIN) $(UPDATE_BIN) $(CGI) $(TEST_BIN)

# Add debugging symbols and optimizations to check for more warnings
debug: CFLAGS += -O -g
debug: $(BINS) teerank-test

# Remove assertions and enable optimizations
release: CFLAGS += -DNDEBUG -O2
release: $(BINS) teerank-test

# Object files
core_objs    = $(patsubst %.c,%.o,$(wildcard core/*.c))
cgi_objs     = $(patsubst %.c,%.o,$(wildcard cgi/*.c) $(wildcard cgi/page/*.c))
update_objs  = $(patsubst %.c,%.o,$(wildcard update/*.c))
upgrade_objs = $(patsubst %.c,%.o,$(wildcard upgrade/*.c))
test_objs    = $(patsubst %.c,%.o,$(wildcard test/*.c))

# Header files
core_headers    = $(wildcard core/*.h core/*.def)
update_headers  = $(wildcard update/*.h)
upgrade_headers = $(wildcard upgrade/*.h)
cgi_headers     = $(wildcard cgi/*.h cgi/*.def)
test_headers    = $(wildcard test/*.h)

# Header files dependencies
$(core_objs):    $(core_headers)
$(update_objs):  $(core_headers) $(update_headers)
$(upgrade_objs): $(core_headers) $(upgrade_headers)
$(cgi_objs):     $(core_headers) $(cgi_headers)
$(test_objs):    $(core_headers) $(test_headers)

# Binaries objects dependencies
$(UPDATE_BIN):  $(core_objs) $(update_objs)
$(UPGRADE_BIN): $(core_objs) $(upgrade_objs)
$(CGI):         $(core_objs) $(cgi_objs)
$(TEST_BIN):    $(core_objs) $(test_objs)

%.o: %.c
	@echo "BUILD $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BINS):
	@echo "LINK $@"
	@$(CC) $(LDFLAGS) -o $@ $^

# Test process
teerank-test: export TEERANK_DB = teerank-test.sqlite3
teerank-test: test/test.sh $(BINS)
	@echo "RUN TEST"
	@cp $< $@
	@./teerank-test || (rm ./teerank-test && false)

#
# Clean
#

clean:
	@rm -f $(core_objs)
	@rm -f $(update_objs)
	@rm -f $(upgrade_objs)
	@rm -f $(cgi_objs)
	@rm -f $(BINS)

#
# Install
#

install: TEERANK_ROOT      = $(prefix)/var/lib/teerank
install: TEERANK_DATA_ROOT = $(prefix)/usr/share/webapps/teerank
install: TEERANK_BIN_ROOT  = $(prefix)/usr/bin
install:
	mkdir -p $(TEERANK_ROOT)
	mkdir -p $(TEERANK_DATA_ROOT)
	mkdir -p $(TEERANK_BIN_ROOT)

	cp $(UPGRADE_BINS) $(TEERANK_BIN_ROOT)
	cp $(UPDATE_BINS) $(TEERANK_BIN_ROOT)
	cp -r $(CGI) assets/* $(TEERANK_DATA_ROOT)

.PHONY: all debug release clean install
