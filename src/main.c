#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "graph.h"
#include "command.h"

#define SERVER_PORT 8080
#define CHUNK_SIZE 512
#define MAX_LINE 1024

static const char *execute_command(const char *command, Graph *graph){
	if(parse_command(command) == CMD_ADD_NODE){

		uint64_t node_id;

		if(parse_add_node(command, &node_id) == -1){
			return "ERROR\n";
		}

		if(graph_add_node(graph, node_id) == false){
			return "ERROR\n";
		}

		graph_print(graph);

		return "OK\n";

	}else if(parse_command(command) == CMD_ADD_EDGE){

		uint64_t from, to, weight;

		if(parse_add_edge(command, &from, &to, &weight) != 0){
			return "ERROR\n";
		}

		if(!graph_add_edge(graph, from, to, weight)){
			return "ERROR\n";
		}

		graph_print(graph);

		return  "OK\n";
	}

	return "ERROR\n";
}

static int send_all(int fd, const char * data, size_t len);

static int server_init(void){
	// server fd

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(server_fd < 0){
		perror("socket");
		return -1;
	}

	// ip + port

	struct sockaddr_in address = {0};

	address.sin_family = AF_INET;
	address.sin_port = htons(SERVER_PORT);

	if(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0){
		perror("inet_pton");
		close(server_fd);
		return -1;
	}

	// server_fd + address

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
			printf("Client disconnected\n");
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
			if(used == sizeof(line)){
                                fprintf(stderr, "Command too long\n");
                                return;
                        }

                        line[used++] = chunk[i];

                        if(chunk[i] == '\n'){
				line[used-1] = '\0';
                                const char * response = execute_command(line, graph);

                        	if(send_all(client_fd, response, strlen(response)) == -1){
					return;
				}

                                used = 0;
                        }
		}
	}
}


static int send_all(int fd, const char * data, size_t len){
	size_t sent = 0;

	while(sent < len){
		ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);

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
			perror("accept");
			close(server_fd);
			return 1;
		}

		printf("Client connected!\n");

		handle_client(client_fd, graph);

		close(client_fd);

		printf("Client disconnected\n");
	}

	graph_destroy(graph);
	close(server_fd);

	return 0;
}
