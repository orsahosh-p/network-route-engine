#include "graph.h"
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#define HASH_CAPACITY 257

void graph_print(const Graph* graph, FILE* output){
	if(graph == NULL){
		fprintf(output, "Graph is NULL\n");
		return;
	}

	fprintf(output, "\n========== Graph ===========\n");
	fprintf(output, "nodes: %zu\n", graph->size);

	for(size_t i = 0; i < graph->size; i++){
		Node *node = graph->node_list[i];

		fprintf(output, "Node %" PRIu64 " -> ", node->id);

		Edge *edge = node->edges;

		if(edge == NULL){
			fprintf(output, "(no edges)");
		}

		while(edge != NULL){
			fprintf(output, "[%" PRIu64 ", weight=%" PRIu64 "]", edge->to->id, edge->weight);

			edge = edge->next;

			if(edge != NULL){
				fprintf(output, " -> ");
			}
		}

		fprintf(output, "\n");
	}

	fprintf(output, "==============================\n\n");
}

Graph* graph_create(size_t capacity){

	if(capacity == 0){
		return NULL;
	}

	Graph* graph = malloc(sizeof(*graph));

	if(graph == NULL){
		return NULL;
	}

	graph->nodes = hash_table_create(HASH_CAPACITY);

	if(graph->nodes == NULL){
		free(graph);
		return NULL;
	}

	graph->node_list = calloc(capacity, sizeof(Node*));
	if(graph->node_list == NULL){
		hash_table_destroy(graph->nodes);
		free(graph);
		return NULL;
	}

	graph->size = 0;
	graph->capacity = capacity;

	return graph;
}

Node *graph_get_node(Graph* graph, uint64_t id){
	if(graph == NULL){
		return NULL;
	}

	return hash_table_get(graph->nodes, id);
}

bool graph_add_node(Graph* graph, uint64_t id){
	if(graph == NULL){
		return false;
	}

	if(graph_get_node(graph, id) != NULL){
		return false;
	}

	if(graph->size == graph->capacity){
		if(graph->capacity > SIZE_MAX / 2 / sizeof(*graph->node_list)){
			return false;
		}

		size_t new_capacity = graph->capacity * 2;

		Node** new_list = realloc(graph->node_list, new_capacity * sizeof(Node*));

		if(new_list == NULL){
			return false;
		}

		graph->node_list = new_list;
		graph->capacity = new_capacity;
	}

	Node* node = malloc(sizeof(*node));
	if(node == NULL){
		return false;
	}

	node->id = id;
	node->edges = NULL;

	if(!hash_table_insert(graph->nodes, id, node)){
		free(node);
		return false;
	}

	graph->node_list[graph->size] = node;
	graph->size++;

	return true;
}

bool graph_add_edge(Graph* graph, uint64_t from, uint64_t to, uint64_t weight){
	if(graph == NULL || from == to){
		return false;
	}

	Node* from_node = graph_get_node(graph, from);
	Node* to_node = graph_get_node(graph, to);

	if(from_node == NULL || to_node == NULL){
		return false;
	}

	for(Edge* edge = from_node->edges; edge != NULL; edge = edge->next){
		if(edge->to == to_node){
			return false;
		}
	}

	Edge* forward = malloc(sizeof(Edge));
	if(forward == NULL){
		return false;
	}

	Edge* backward = malloc(sizeof(Edge));
	if(backward == NULL){
		free(forward);
		return false;
	}

	// from -> to
	forward->to = to_node;
	forward->weight = weight;
	forward->next = from_node->edges;
	from_node->edges = forward;
	// to -> from
	backward->to = from_node;
	backward->weight = weight;
	backward->next = to_node->edges;

	to_node->edges = backward;

	return true;
}

void graph_destroy(Graph *graph){
	if(graph == NULL){
		return;
	}

	for(size_t i = 0; i < graph->size; i++){
		Node* node = graph->node_list[i];

		Edge *current = node->edges;

		while(current != NULL){
			Edge* next = current->next;

			free(current);

			current = next;
		}

		free(node);
	}

	free(graph->node_list);
	hash_table_destroy(graph->nodes);
	free(graph);
}

