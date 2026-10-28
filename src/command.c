#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>

#include "command.h"
static int parse_uint64(const char **input, uint64_t *result){
	if(input == NULL || *input == NULL || result == NULL){
		return -1;
	}

	const char *p = *input;

	if(!isdigit((unsigned char)*p)){
		return -1;
	}

	errno = 0;

	char *end;
	unsigned long long value = strtoull(p, &end, 10);

	if(errno == ERANGE || end == p || value > UINT64_MAX){
		return -1;
	}

	*result = (uint64_t)value;
	*input = end;

	return 0;
}

CommandType parse_command(const char *input){

	if(input == NULL || *input == '\0'){
		return CMD_UNKNOWN;
	}

	if(strncmp(input, "ADD_NODE ", 9) == 0){
		return CMD_ADD_NODE;
	}else if(strcmp(input, "PRINT") == 0){
		return CMD_PRINT;
	}else if(strcmp(input, "PING") == 0){
		return CMD_PING;
	}else if(strncmp(input, "ADD_EDGE ", 9) == 0){
		return CMD_ADD_EDGE;
	}else if(strncmp(input, "GET_NODE ", 9) == 0){
		return CMD_GET_NODE;
	}else{
		return CMD_UNKNOWN;
	}

}

static int parse_node_id(const char *input, const char *prefix, uint64_t *node_id){
	if(input == NULL || node_id == NULL){
		return -1;
	}

	size_t length = strlen(prefix);
	if(strncmp(input, prefix, length) != 0){
		return -1;
	}

	const char *p = input + length;
	uint64_t id;

	if(parse_uint64(&p, &id) != 0){
		return -1;
	}

	if(*p != '\0'){
		return -1;
	}

	*node_id = id;

	return 0;
}

int parse_add_node(const char *input, uint64_t *node_id){
	return parse_node_id(input, "ADD_NODE ", node_id);
}

int parse_get_node(const char *input, uint64_t *node_id){
	return parse_node_id(input, "GET_NODE ", node_id);
}

int parse_add_edge(const char *input, uint64_t *from, uint64_t *to, uint64_t *weight){
	if(input == NULL || from == NULL || to == NULL || weight == NULL){
		return -1;
	}

	if(strncmp(input, "ADD_EDGE ", 9) != 0){
		return -1;
	}

	const char *p = input + 9;

	uint64_t src, dst, w;

	if(parse_uint64(&p, &src) != 0){
		return -1;
	}

	if(*p != ' '){
		return -1;
	}

	p++;

	if(parse_uint64(&p, &dst) != 0){
		return -1;
	}

	if(*p != ' '){
		return -1;
	}

	p++;

	if(parse_uint64(&p, &w) != 0){
		return -1;
	}

	if(*p != '\0'){
		return -1;
	}

	*from = src;
	*to = dst;
	*weight = w;

	return 0;
}
