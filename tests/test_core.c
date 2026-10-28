#include <assert.h>
#include <string.h>
#include "command.h"
#include "graph.h"

static void test_parser(void){
	uint64_t id = 42, from, to, weight;
	assert(parse_command(NULL) == CMD_UNKNOWN);
	assert(parse_command("") == CMD_UNKNOWN);
	assert(parse_command("PING") == CMD_PING);
	assert(parse_command("PRINT") == CMD_PRINT);
	assert(parse_command("PRINT extra") == CMD_UNKNOWN);
	assert(parse_command("PING extra") == CMD_UNKNOWN);
	assert(parse_add_node("ADD_NODE 0", &id) == 0 && id == 0);
	assert(parse_add_node("ADD_NODE 18446744073709551615", &id) == 0 && id == UINT64_MAX);
	const char* invalid[] = {
		"ADD_NODE ", "ADD_NODE -1", "ADD_NODE +1", "ADD_NODE  1",
		"ADD_NODE 1 ", "ADD_NODE 1x", "ADD_NODE 18446744073709551616", "ADD_EDGE 1"
	};
	for(size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++){
		id = 42;
		assert(parse_add_node(invalid[i], &id) == -1 && id == 42);
	}
	assert(parse_add_node(NULL, &id) == -1);
	assert(parse_add_node("ADD_NODE 1", NULL) == -1);
	assert(parse_add_edge("ADD_EDGE 1 2 0", &from, &to, &weight) == 0);
	assert(from == 1 && to == 2 && weight == 0);
	const char* edges[] = {
		"ADD_EDGE 1", "ADD_EDGE 1 2", "ADD_EDGE 1 2 ", "ADD_EDGE 1  2 3",
		"ADD_EDGE 1 2 -1", "ADD_EDGE 1 2 3x", "ADD_EDGE 1 2 3 4"
	};
	for(size_t i = 0; i < sizeof(edges) / sizeof(*edges); i++){
		from = to = weight = 42;
		assert(parse_add_edge(edges[i], &from, &to, &weight) == -1);
		assert(from == 42 && to == 42 && weight == 42);
	}
}

static void test_get_node_parser(void){
	uint64_t id;
	assert(parse_command("GET_NODE 1") == CMD_GET_NODE);
	assert(parse_get_node("GET_NODE 0", &id) == 0 && id == 0);
	assert(parse_get_node("GET_NODE 0001", &id) == 0 && id == 1);
	assert(parse_get_node("GET_NODE 18446744073709551615", &id) == 0 && id == UINT64_MAX);
	assert(parse_get_node(NULL, &id) == -1);
	assert(parse_get_node("GET_NODE 1", NULL) == -1);
	const char* invalid[] = {
		"GET_NODE", "GET_NODE ", "GET_NODE -1", "GET_NODE +1", "GET_NODE  1",
		"GET_NODE 1 ", "GET_NODE 1x", "GET_NODE 1 2", "GET_NODE 18446744073709551616",
		"ADD_NODE 1", "GET_NODE 1\r"
	};
	for(size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++){
		id = 42;
		assert(parse_get_node(invalid[i], &id) == -1 && id == 42);
	}
}

static void test_table(void){
	assert(hash_table_create(0) == NULL);
	assert(!hash_table_insert(NULL, 0, NULL));
	assert(!hash_table_remove(NULL, 0));
	assert(hash_table_get(NULL, 0) == NULL);
	hash_table_destroy(NULL);
	HashTable* table = hash_table_create(1);
	assert(table != NULL);
	int values[] = {10, 20, 30, 40};
	for(uint64_t i = 0; i < 3; i++){
		assert(hash_table_insert(table, i, &values[i]));
	}
	assert(table->size == 3);
	assert(hash_table_insert(table, 1, &values[3]));
	assert(table->size == 3 && hash_table_get(table, 1) == &values[3]);
	assert(!hash_table_remove(table, 99));
	assert(hash_table_remove(table, 1)); // middle
	assert(hash_table_remove(table, 0)); // tail
	assert(hash_table_remove(table, 2)); // head
	assert(table->size == 0 && hash_table_get(table, 1) == NULL);
	hash_table_destroy(table);
}

static void test_graph(void){
	assert(graph_create(0) == NULL);
	assert(!graph_add_node(NULL, 1));
	assert(!graph_add_edge(NULL, 1, 2, 3));
	graph_destroy(NULL);
	Graph* graph = graph_create(1);
	assert(graph != NULL);
	for(uint64_t i = 0; i < 600; i++){
		assert(graph_add_node(graph, i));
		assert(graph_get_node(graph, i) == graph->node_list[i]);
	}
	assert(graph->size == 600 && graph->capacity >= 600);
	assert(!graph_add_node(graph, 0));
	assert(graph_add_edge(graph, 0, 257, 10)); // IDs collide in the table
	Node* a = graph_get_node(graph, 0);
	Node* b = graph_get_node(graph, 257);
	assert(a->edges->to == b && b->edges->to == a);
	assert(a->edges->weight == 10 && b->edges->weight == 10);
	assert(!graph_add_edge(graph, 257, 0, 20));
	assert(!graph_add_edge(graph, 0, 0, 20));
	assert(!graph_add_edge(graph, 0, 999, 20));
	assert(a->edges->next == NULL && b->edges->next == NULL);
	graph_destroy(graph);
}

int main(void){
	test_parser();
	test_get_node_parser();
	test_table();
	test_graph();
	return 0;
}
