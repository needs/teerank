CFLAGS += -Isrc -Wall -Werror -O -std=c89 -D_POSIX_C_SOURCE=200809L -g
BINS_HTML = $(addprefix teerank-html-,about player-page rank-page clan-page elo-graph search)
BINS = $(addprefix teerank-,add-new-servers update-servers update-players update-clans compute-ranks paginate-ranks remove-offline-servers repair upgrade-4-to-5) $(BINS_HTML)
SCRIPTS = $(addprefix teerank-,create-database upgrade-0-to-1 upgrade-1-to-2 upgrade-2-to-3 upgrade-3-to-4 upgrade update)
CGI = teerank.cgi

.PHONY: all clean install

all: $(BINS) $(SCRIPTS) $(CGI)

# Header file dependancies
src/config.o: src/config.h
src/player.o: src/player.h
src/cgi.o: src/cgi.h
src/delta.o: src/delta.h
src/elo.o: src/elo.h
src/network.o: src/network.h
src/pool.o: src/pool.h
src/route.o: src/route.h
src/server.o: src/server.h
src/clan.o: src/clan.h

# All binaries share the same configuration
$(BINS): src/config.o
$(CGI): src/config.o
$(BINS_HTML): src/html/html.o

#
# Binaries
#

teerank-add-new-servers: src/add-new-servers.o src/network.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-update-servers: src/update-servers.o src/network.o src/pool.o src/delta.o src/server.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-update-players: src/update-players.o src/delta.o src/elo.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^ -lm

teerank-update-clans: src/update-clans.o src/player.o src/historic.o src/clan.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-compute-ranks: src/compute-ranks.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-paginate-ranks: src/paginate-ranks.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-remove-offline-servers: src/remove-offline-servers.o src/server.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-repair: src/repair.o src/player.o src/historic.o src/clan.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-clan-page: src/html/clan-page.o src/player.o src/historic.o src/clan.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-rank-page: src/html/rank-page.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-player-page: src/html/player-page.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-about: src/html/about.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-search: src/html/search.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-html-elo-graph: src/html/elo_graph.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-upgrade-4-to-5: src/upgrade/4-to-5.o src/player.o src/historic.o
	$(CC) -o $@ $(CFLAGS) $^

#
# Scripts
#

teerank-create-database: src/script-header.inc.sh src/create-database.sh
	cat $^ >$@ && chmod +x $@

teerank-upgrade: src/script-header.inc.sh src/upgrade.sh
	cat $^ >$@ && chmod +x $@

teerank-upgrade-0-to-1: src/script-header.inc.sh src/upgrade/0-to-1.sh
	cat $^ >$@ && chmod +x $@
teerank-upgrade-1-to-2: src/script-header.inc.sh src/upgrade/1-to-2.sh
	cat $^ >$@ && chmod +x $@
teerank-upgrade-2-to-3: src/script-header.inc.sh src/upgrade/2-to-3.sh
	cat $^ >$@ && chmod +x $@
teerank-upgrade-3-to-4: src/script-header.inc.sh src/upgrade/3-to-4.sh
	cat $^ >$@ && chmod +x $@

teerank-update: src/update.sh
	cp $< $@

#
# CGI
#

$(CGI): src/cgi.o src/route.o
	$(CC) -o $@ $(CFLAGS) $^

#
# Phony targets
#

clean:
	rm -f src/*.o src/html/*.o $(BINS) $(SCRIPTS) $(CGI)

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
