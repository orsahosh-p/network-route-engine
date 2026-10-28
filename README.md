# Network Route Engine

A small C11 learning project for Linux: a TCP server maintains a weighted,
undirected graph in memory. It also builds on macOS using POSIX sockets.

## Build and run

Requires a C11 compiler and Make. Python 3.8+ is needed only for network tests.

From the repository directory, build and start the server:

```sh
make
./route-server
```

The server listens on `127.0.0.1:8080`. In another terminal, open the same
repository directory and start the interactive client:

```sh
./route-client
```

Enter commands at the `route>` prompt; there is no need to restart the client
for each command:

```text
route> PING
PONG
route> ADD_NODE 1
OK
route> ADD_NODE 2
OK
route> ADD_EDGE 1 2 10
OK
route> GET_NODE 1
NODE 1
route> PRINT

========== Graph ===========
nodes: 2
Node 1 -> [2, weight=10]
Node 2 -> [1, weight=10]
==============================

route> QUIT
```

`PRINT` reuses the adjacency-list display from `graph_print()`. Each undirected
edge appears at both endpoints. An empty graph displays `nodes: 0`.

`QUIT` or Ctrl+D closes the client session. Invalid commands print `ERROR` and
allow you to continue. The server handles one client at a time; close the
interactive session before opening another client.

For a single command, use a quoted argument:

```sh
./route-client "PING"
./route-client "GET_NODE 1"
./route-client "PRINT"
```

In this mode, the client prints the response and exits. An `ERROR` response
produces a nonzero exit status. `QUIT` is a local interactive command, not a
server command.

Stop the server with Ctrl+C. The graph exists only in memory: restarting the
server discards all nodes and edges. After rebuilding changed code, restart the
server and client to use the new binaries.

## Project structure

```text
network-route-engine/
├── include/
│   ├── command.h
│   ├── graph.h
│   └── hash_table.h
├── src/
│   ├── main.c
│   ├── command.c
│   ├── graph.c
│   └── hash_table.c
├── client/
│   └── client.c
├── tests/
│   ├── test_core.c
│   └── test_network.py
├── Makefile
└── README.md
```

The application connects three main parts: command parsing, graph operations
and TCP networking. The hash table supports node lookup inside the graph.

## Protocol

Each command ends in `\n`. Responses are either one line or the framed multiline
output of `PRINT`. TCP reads do not define message boundaries:
the server accumulates incomplete commands and processes multiple lines in one read.

| Command | Response | Behavior |
| --- | --- | --- |
| `PING` | `PONG` | Check connectivity |
| `PRINT` | `GRAPH`, formatted graph, `END` | Stream the whole graph |
| `ADD_NODE id` | `OK` or `ERROR` | Insert a new node |
| `GET_NODE id` | `NODE id` or `ERROR` | Look up an existing node without modifying the graph |
| `ADD_EDGE from to weight` | `OK` or `ERROR` | Insert an undirected edge between existing nodes |

`PRINT` uses a multiline response: `GRAPH\n`, the formatted output of
`graph_print()`, then `END\n`. Every node shows its adjacency list, so an
undirected edge appears at both endpoints. Output is streamed, and long adjacency
lines are read in chunks by the client. An empty graph displays `nodes: 0`.

Numbers are decimal unsigned 64-bit integers, including zero. The grammar uses
exactly one space between arguments; signs, trailing spaces, extra arguments and
CRLF are rejected. Node IDs must be unique. Duplicate edges and self-loops are
rejected. Both endpoints must exist before an edge is added.

A command can contain at most 1023 bytes before the newline. Embedded NUL bytes
and longer commands cause `ERROR` followed by disconnection. Other invalid
commands produce `ERROR` and leave the connection open. An incomplete command at
EOF is discarded.

`GET_NODE` returns the stored decimal ID, without leading zeros. It does not
include edges. Missing nodes and malformed commands return `ERROR`.
`HELLO` also returns `ERROR`.

## Implementation

### Command parser: `include/command.h`, `src/command.c`

- `parse_command()` classifies the command prefix; it does not validate arguments.
- `parse_add_node()`, `parse_get_node()` and `parse_add_edge()` validate the complete command.
- Node commands share a private ID parser to keep validation consistent.
- Private `parse_uint64()` converts a number using `strtoull()` and checks range.
- Argument parsers return `0` on success and `-1` on failure. Output arguments
  are assigned only after the entire command has been validated.

### Graph: `include/graph.h`, `src/graph.c`

`Graph` stores pointers to the same nodes in two containers:

- `nodes`: a hash table for lookup by ID.
- `node_list`: a growable array for iteration and destruction.

Each node owns a linked list of outgoing `Edge` records. An undirected connection
allocates two records, one for each direction. Both allocations succeed before
lists are modified. An edge points to a node; it does not own that node.

The graph owns all nodes and edges. `graph_destroy()` frees edge lists, nodes,
the node array and the hash table. The node array doubles when full; growth is
checked for allocation-size overflow. The hash table uses 257 fixed buckets.
`graph_print(graph, output)` writes to a `FILE*`: pass `stdout` for local
debugging. The server reuses it for `PRINT` through a socket-backed stream.

### Hash table: `include/hash_table.h`, `src/hash_table.c`

A general `uint64_t -> void*` container. Bucket selection is `key % capacity`;
collisions form linked lists. Insertion replaces an existing value or prepends a
new entry. Removal walks links using a pointer to the link, so removing the first
entry and removing a later entry use the same code.

The table owns its buckets and entries, but **does not own values**. Replacing,
removing or destroying an entry never frees the pointed-to value. The graph
handles node ownership separately.

### Networking and application: `src/main.c`, `client/client.c`

- `server_init()`: socket creation, address reuse and binding.
- `main()`: listening, graph creation, accepting and closing clients.
- `handle_client()`: receiving bytes, framing lines and returning responses.
- `execute_command()`: joins the parser to graph operations.
- `send_all()`: retries interrupted writes and completes partial writes.

The client accepts a command as one quoted argument, or runs an interactive
session with no arguments. It reads complete response lines even if TCP splits
them, and consumes the entire graph through `END` before sending the next command.
Both executables ignore SIGPIPE and handle write failures as ordinary errors.

## Tests

```sh
make test
```

Core tests cover parser limits and malformed input, hash collisions, replacement,
removal, graph growth, duplicate nodes, symmetric edges and invalid connections.
Network tests build their own server and client on port 18080 and cover
split/batched commands, errors, line limits, reconnection, PRINT and interactive
sessions. Port 18080 must be available; a normal server on 8080 can keep running.

To rebuild all test executables with AddressSanitizer and
UndefinedBehaviorSanitizer:

```sh
make -B test TEST_FLAGS="-fsanitize=address,undefined"
```

The current changes were built and tested on macOS with Clang, including the
sanitizer run. Linux is the target platform; this revision has not yet been
verified in a Linux environment.

## Current limits

- One client is handled at a time; an idle client delays all later clients.
- No persistence, routing algorithms, authentication or encryption.
- Rejected commands use the same `ERROR` response without detailed reasons.
- The hash table does not resize; long collision chains slow down lookup.
- No graceful signal-driven shutdown; the OS reclaims resources on Ctrl+C.
  Graph cleanup runs when the accept loop exits on an error.

The project deliberately keeps C11, plain functions and the existing module
layout rather than adding concurrency or a new framework.
