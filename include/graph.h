#ifndef GRAPH_H
#define GRAPH_H

#include "hash_table.h"
#include <stdlib.h>

typedef struct Node Node;
typedef struct Edge Edge;

struct Node{
	uint64_t id;
	Edge* edges;
};

struct Edge{
	Node* to;
	Edge* next;
	uint64_t weight;
};

typedef struct Graph{
	HashTable* nodes;

	Node** node_list;
	size_t size;
	size_t capacity;
}Graph;

Graph* graph_create(size_t capacity);
void graph_destroy(Graph * graph);

bool graph_add_node(Graph *graph, uint64_t id);
Node* graph_get_node(Graph *graph, uint64_t id);

bool graph_add_edge(Graph *graph, uint64_t from, uint64_t to, uint64_t weight);

void graph_print(const Graph *graph);

#endif
