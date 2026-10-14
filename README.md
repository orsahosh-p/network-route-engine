# Network Route Engine

A lightweight network routing engine written in **C11 for Linux**.

This project focuses on data structures, graph algorithms, TCP networking, and Linux system programming.

## Features

- Hash Table (Separate Chaining)
- Weighted Undirected Graph (Adjacency List)
- Dynamic Node and Edge Management
- TCP Server and Client
- TCP Echo Communication
- POSIX Socket API

## Technologies

- C11
- Linux (Debian)
- GCC
- Make
- TCP/IP
- Git

## Project Structure

```text
network-route-engine/
├── include/
│   ├── graph.h
│   └── hash_table.h
├── src/
│   ├── main.c
│   ├── graph.c
│   └── hash_table.c
├── client/
│   └── main.c
├── Makefile
└── README.md
```

## Build

Compile the server:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude \
    src/*.c -o route-server
```

Compile the client:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic \
    client/main.c -o route-client
```

## Run

Start the server:

```bash
./route-server
```

Start the client in another terminal:

```bash
./route-client
```

Server address: `127.0.0.1:8080`

## Example Output

Server:

```text
Network Route Engine Server
Server listening on 127.0.0.1:8080
Client connected!
Received: HELLO
```

Client:

```text
Network Route Engine Client
Connected to server!
Sent: HELLO
Received: HELLO
```

## Architecture

```text
TCP Client
    |
    | TCP/IP
    |
TCP Server
    |
    v
Routing Engine (planned)
    |
    +-- Hash Table
    |
    +-- Weighted Graph
    |
    +-- BFS / DFS / Dijkstra (planned)
```

The hash table provides node lookup by ID.

The graph uses adjacency lists to represent weighted connections between nodes.

The current TCP implementation supports basic echo communication.

## Roadmap

- [x] Project Initialization
- [x] Hash Table
- [x] Graph Data Structures
- [x] TCP Server
- [x] TCP Client
- [x] TCP Echo Communication
- [ ] TCP Message Framing
- [ ] BFS
- [ ] DFS
- [ ] Dijkstra's Algorithm
- [ ] Routing Protocol
- [ ] Unix Domain Sockets
- [ ] Testing and Improvements

## Learning Objectives

- C Programming and Memory Management
- Data Structures and Algorithms
- Graph Theory
- Linux System Calls
- TCP/IP Networking
- Client-Server Architecture
- Software Engineering

## Status

**Work in Progress**

Built as a practical Computer Science and Systems Programming learning project.
