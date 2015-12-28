CFLAGS = -Wall -Werror -O -ansi -D_POSIX_C_SOURCE=200809L -g
BINS = $(addprefix teerank-,add-new-servers update-servers generate-index update-players update-clans generate-clan-page compute-ranks generate-rank-page generate-about paginate-ranks init-database update)
CGI = teerank.cgi

.PHONY: all clean install

all: $(BINS) $(CGI)

# All binaries share the same configuration
$(BINS): src/config.o
$(CGI): src/config.o

teerank-add-new-servers: src/add-new-servers.o src/network.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-update-servers: src/update-servers.o src/network.o src/pool.o src/delta.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-generate-index: src/generate-index.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-update-players: src/update-players.o src/delta.o src/elo.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^ -lm

teerank-update-clans: src/update-clans.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-generate-clan-page: src/generate-clan-page.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-compute-ranks: src/compute-ranks.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-generate-rank-page: src/generate-rank-page.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-paginate-ranks: src/paginate-ranks.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-init-database: src/init-database.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-generate-about: src/generate-about.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

teerank-update: src/update.sh
	cp $< $@

$(CGI): src/cgi.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

clean:
	rm -f src/*.o $(BINS) $(CGI)

install: TEERANK_ROOT       = $(prefix)/var/lib/teerank
install: TEERANK_CACHE_ROOT = $(prefix)/var/cache/teerank
install: TEERANK_DATA_ROOT  = $(prefix)/usr/share/webapps/teerank
install: TEERANK_BIN_ROOT   = $(prefix)/usr/bin
install:
	mkdir -p $(TEERANK_ROOT)
	mkdir -p $(TEERANK_CACHE_ROOT)
	mkdir -p $(TEERANK_DATA_ROOT)
	mkdir -p $(TEERANK_BIN_ROOT)

	cp $(BINS) $(TEERANK_BIN_ROOT)
	cp -r $(CGI) style.css images $(TEERANK_DATA_ROOT)
