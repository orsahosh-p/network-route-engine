#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

typedef enum {
	CMD_PING,
	CMD_PRINT,
	CMD_ADD_NODE,
	CMD_ADD_EDGE,
	CMD_GET_NODE,
	CMD_UNKNOWN
} CommandType;

CommandType parse_command(const char *input);
int parse_add_node(const char *input, uint64_t *node_id);
int parse_get_node(const char *input, uint64_t *node_id);
int parse_add_edge(const char *input, uint64_t *from, uint64_t *to, uint64_t *weight);

#endif
