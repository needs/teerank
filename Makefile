CFLAGS = -Wall -Werror -O -ansi -D_POSIX_C_SOURCE=200809L -g
BINS = add_new_servers update_servers generate_index update_players update_clans generate_clan_page compute_ranks generate_rank_page paginate_ranks teerank.cgi init_database

.PHONY: all clean

all: $(BINS)

# All binaries share the same configuration
$(BINS): src/config.o

add_new_servers: src/add_new_servers.o src/network.o
	$(CC) -o $@ $(CFLAGS) $^

update_servers: src/update_servers.o src/network.o src/pool.o src/delta.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

generate_index: src/generate_index.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

update_players: src/update_players.o src/delta.o src/elo.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^ -lm

update_clans: src/update_clans.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

generate_clan_page: src/generate_clan_page.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

compute_ranks: src/compute_ranks.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

generate_rank_page: src/generate_rank_page.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

paginate_ranks: src/paginate_ranks.o
	$(CC) -o $@ $(CFLAGS) $^

teerank.cgi: src/cgi.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

init_database: src/init_database.o
	$(CC) -o $@ $(CFLAGS) $^

clean:
	rm -f src/*.o $(BINS)
