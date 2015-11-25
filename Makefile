CFLAGS = -Wall -Werror -O -ansi -D_POSIX_C_SOURCE=200809L -g
BINS = add_new_servers update_servers generate_index update_players

.PHONY: all clean

all: $(BINS)

add_new_servers: src/add_new_servers.o src/network.o
	$(CC) -o $@ $(CFLAGS) $^

update_servers: src/update_servers.o src/network.o src/pool.o src/delta.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

generate_index: src/generate_index.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^

update_players: src/update_players.o src/delta.o src/elo.o src/io.o
	$(CC) -o $@ $(CFLAGS) $^ -lm

clean:
	rm -f src/*.o $(BINS)
