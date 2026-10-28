CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude

SERVER = route-server
CLIENT = route-client

SERVER_SRC = $(wildcard src/*.c)
CLIENT_SRC = $(wildcard client/*.c)
HEADERS = $(wildcard include/*.h)
CORE_SRC = src/command.c src/graph.c src/hash_table.c
TEST = build/test-core
TEST_FLAGS ?=

.PHONY: all clean run test

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $@

$(CLIENT): $(CLIENT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $@

run: $(SERVER)
	./$(SERVER)

$(TEST): tests/test_core.c $(CORE_SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) $(TEST_FLAGS) tests/test_core.c $(CORE_SRC) -o $@

build/route-server-test: $(SERVER_SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) $(TEST_FLAGS) -DSERVER_PORT=18080 $(SERVER_SRC) -o $@

build/route-client-test: $(CLIENT_SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) $(TEST_FLAGS) -DSERVER_PORT=18080 $(CLIENT_SRC) -o $@

# Test binaries use port 18080, leaving the normal server on 8080 alone.
test: $(TEST) build/route-server-test build/route-client-test
	./$(TEST)
	python3 tests/test_network.py

clean:
	rm -f $(SERVER) $(CLIENT) $(TEST) build/route-server-test build/route-client-test
