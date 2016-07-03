CFLAGS += -lm -Icore -Icgi -Wall -Werror -O -std=c89 -D_POSIX_C_SOURCE=200809L -g

BINS_HTML = $(addprefix teerank-html-,about player-page rank-page clan-page graph search)
BINS = $(addprefix teerank-,add-new-servers update-servers update-players update-clans compute-ranks remove-offline-servers repair upgrade-4-to-5) $(BINS_HTML)
SCRIPTS = $(addprefix teerank-,create-database upgrade-0-to-1 upgrade-1-to-2 upgrade-2-to-3 upgrade-3-to-4 upgrade update)
CGI = teerank.cgi

.PHONY: all clean install

all: $(BINS) $(SCRIPTS) $(CGI)

# Header file dependancies
core_objs = $(patsubst %.c,%.o,$(wildcard core/*.c))
page_objs = cgi/html.o

core_headers = $(wildcard core/*.h)
page_headers = cgi/html.h

$(core_objs): $(core_headers)
$(page_objs): $(page_headers)

#
# Binaries
#

teerank-add-new-servers: builtin/add-new-servers.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-update-servers: builtin/update-servers.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-update-players: builtin/update-players.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^ -lm

teerank-update-clans: builtin/update-clans.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-compute-ranks: builtin/compute-ranks.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-remove-offline-servers: builtin/remove-offline-servers.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-repair: builtin/repair.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-clan-page: cgi/page/clan-page.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-rank-page: cgi/page/rank-page.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-player-page: cgi/page/player-page.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-about: cgi/page/about.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-search: cgi/page/search.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-graph: cgi/page/graph.o $(core_objs) $(page_objs)
	$(CC) -o $@ $(CFLAGS) $^

teerank-upgrade-4-to-5: upgrade/4-to-5/4-to-5.o upgrade/4-to-5/ranks.o upgrade/4-to-5/players.o upgrade/4-to-5/servers.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

#
# Scripts
#

teerank-create-database: core/script-header.inc.sh builtin/create-database.sh
	cat $^ >$@ && chmod +x $@

teerank-upgrade: core/script-header.inc.sh builtin/upgrade.sh
	cat $^ >$@ && chmod +x $@

teerank-upgrade-0-to-1: core/script-header.inc.sh upgrade/0-to-1.sh
	cat $^ >$@ && chmod +x $@
teerank-upgrade-1-to-2: core/script-header.inc.sh upgrade/1-to-2.sh
	cat $^ >$@ && chmod +x $@
teerank-upgrade-2-to-3: core/script-header.inc.sh upgrade/2-to-3.sh
	cat $^ >$@ && chmod +x $@
teerank-upgrade-3-to-4: core/script-header.inc.sh upgrade/3-to-4.sh
	cat $^ >$@ && chmod +x $@

teerank-update: builtin/update.sh
	cp $< $@

#
# CGI
#

$(CGI): cgi/cgi.o cgi/route.o $(core_objs)
	$(CC) -o $@ $(CFLAGS) $^

#
# Phony targets
#

clean:
	rm -f core/*.o builtin/*.o cgi/*.o cgi/page/*.o $(BINS) $(SCRIPTS) $(CGI)

#
# Install
#

install: TEERANK_ROOT       = $(prefix)/var/lib/teerank
install: TEERANK_CACHE_ROOT = $(prefix)/var/cache/teerank
install: TEERANK_DATA_ROOT  = $(prefix)/usr/share/webapps/teerank
install: TEERANK_BIN_ROOT   = $(prefix)/usr/bin
install:
	mkdir -p $(TEERANK_ROOT)
	mkdir -p $(TEERANK_CACHE_ROOT)
	mkdir -p $(TEERANK_DATA_ROOT)
	mkdir -p $(TEERANK_BIN_ROOT)

	cp $(BINS) $(SCRIPTS) $(TEERANK_BIN_ROOT)
	cp -r $(CGI) style.css images $(TEERANK_DATA_ROOT)
