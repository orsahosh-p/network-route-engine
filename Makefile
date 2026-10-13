CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude

SERVER = route-server
CLIENT = route-client

SERVER_SRC = $(wildcard src/*.c)
CLIENT_SRC = $(wildcard client/*.c)

.PHONY: all clean run

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $@

run: $(SERVER)
	./$(SERVER)

clean: 
	rm -f $(SERVER) $(CLIENT)
