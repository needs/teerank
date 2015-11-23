CFLAGS = -Wall -Werror -O -ansi -D_POSIX_C_SOURCE=200809L -g
BINS = add_new_servers update_servers update_players_elo generate_index

.PHONY: all clean

all: $(BINS)

add_new_servers: src/add_new_servers.o src/network.o
	$(CC) -o $@ $(CFLAGS) $^

update_servers: src/update_servers.o src/network.o src/pool.o
	$(CC) -o $@ $(CFLAGS) $^

update_players_elo: src/update_players_elo.o
	$(CC) -o $@ $(CFLAGS) -lm $^

generate_index: src/generate_index.o
	$(CC) -o $@ $(CFLAGS) $^

clean:
	rm -f src/*.o $(BINS)
