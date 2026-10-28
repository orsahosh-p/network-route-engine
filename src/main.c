#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <inttypes.h>

#include "graph.h"
#include "command.h"

#ifndef SERVER_PORT
#define SERVER_PORT 8080
#endif
#define CHUNK_SIZE 512
#define MAX_LINE 1024

static const char *execute_command(const char *command, Graph *graph, char *response, size_t capacity){
	CommandType type = parse_command(command);

	if(type == CMD_PING){
		return "PONG\n";
	}

	if(type == CMD_GET_NODE){
		uint64_t node_id;
		if(parse_get_node(command, &node_id) != 0){
			return "ERROR\n";
		}

		Node* node = graph_get_node(graph, node_id);
		if(node == NULL){
			return "ERROR\n";
		}

		int length = snprintf(response, capacity, "NODE %" PRIu64 "\n", node->id);
		if(length < 0 || (size_t)length >= capacity){
			return "ERROR\n";
		}
		return response;
	}

	if(type == CMD_ADD_NODE){

		uint64_t node_id;

		if(parse_add_node(command, &node_id) != 0){
			return "ERROR\n";
		}

		if(!graph_add_node(graph, node_id)){
			return "ERROR\n";
		}

		return "OK\n";

	}else if(type == CMD_ADD_EDGE){

		uint64_t from, to, weight;

		if(parse_add_edge(command, &from, &to, &weight) != 0){
			return "ERROR\n";
		}

		if(!graph_add_edge(graph, from, to, weight)){
			return "ERROR\n";
		}

		return "OK\n";
	}

	return "ERROR\n";
}

static int send_all(int fd, const char * data, size_t len);

// A duplicated descriptor lets stdio write to the socket without owning client_fd.
static int send_graph(int fd, const Graph* graph){
	int output_fd = dup(fd);
	if(output_fd < 0){
		return -1;
	}

	FILE* output = fdopen(output_fd, "w");
	if(output == NULL){
		close(output_fd);
		return -1;
	}

	fputs("GRAPH\n", output);
	graph_print(graph, output);
	fputs("END\n", output);
	int status = ferror(output) ? -1 : 0;
	if(fclose(output) != 0){
		status = -1;
	}
	return status;
}

static int server_init(void){
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(server_fd < 0){
		perror("socket");
		return -1;
	}

	int reuse = 1;
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
		perror("setsockopt");
		close(server_fd);
		return -1;
	}

	struct sockaddr_in address = {0};

	address.sin_family = AF_INET;
	address.sin_port = htons(SERVER_PORT);

	if(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0){
		perror("inet_pton");
		close(server_fd);
		return -1;
	}

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
		perror("bind");
		close(server_fd);
		return -1;
	}

	return server_fd;
}

static void handle_client(int client_fd, Graph* graph){

	char chunk[CHUNK_SIZE];
	char line[MAX_LINE];
	size_t used = 0;

	while(1){
		ssize_t n = recv(client_fd, chunk, sizeof(chunk), 0);

		if(n == 0){
			break;
		}

		if(n < 0){
			if(errno == EINTR){
				continue;
			}

			perror("recv");
			break;
		}

		for(ssize_t i = 0; i < n; i++){
			if(chunk[i] == '\n'){
				line[used] = '\0';
				if(parse_command(line) == CMD_PRINT){
					if(send_graph(client_fd, graph) != 0){
						return;
					}
					used = 0;
					continue;
				}
				char buffer[64];
				const char* response = execute_command(line, graph, buffer, sizeof(buffer));
				if(send_all(client_fd, response, strlen(response)) != 0){
					return;
				}
				used = 0;
			}else if(chunk[i] == '\0' || used == sizeof(line) - 1){
				send_all(client_fd, "ERROR\n", 6);
				return;
			}else{
				line[used++] = chunk[i];
			}
		}
	}
}


static int send_all(int fd, const char * data, size_t len){
	size_t sent = 0;

	while(sent < len){
		ssize_t n = send(fd, data + sent, len - sent, 0);

		if(n < 0){
			if(errno == EINTR){
				continue;
			}

			perror("send");
			return -1;
		}

		if(n == 0){
			return -1;
		}

		sent += (size_t)n;
	}

	return 0;
}


int main(void){
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
		perror("signal");
		return EXIT_FAILURE;
	}

	printf("Network Route Engine Server\n");

	int server_fd = server_init();
	if(server_fd == -1){
		return 1;
	}

	if(listen(server_fd, 10) < 0){
		perror("listen");
		close(server_fd);
		return 1;
	}

	printf("Server listening on 127.0.0.1:%d\n", SERVER_PORT);
	fflush(stdout);

	Graph* graph = graph_create(16);

	if(graph == NULL){
		close(server_fd);
		return 1;
	}

	while(1){

		int client_fd = accept(server_fd, NULL, NULL);

		if(client_fd == -1){
			if(errno == EINTR){
				continue;
			}
			perror("accept");
			break;
		}

		printf("Client connected!\n");

		handle_client(client_fd, graph);

		close(client_fd);

		printf("Client disconnected\n");
	}

	graph_destroy(graph);
	close(server_fd);

	return EXIT_FAILURE;
}
