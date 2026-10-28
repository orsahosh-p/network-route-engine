#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef SERVER_PORT
#define SERVER_PORT 8080
#endif
#define MAX_COMMAND 1023

static int send_all(int fd, const char* data, size_t length){
	while(length > 0){
		ssize_t n = send(fd, data, length, 0);
		if(n < 0 && errno == EINTR){
			continue;
		}
		if(n <= 0){
			return -1;
		}
		data += n;
		length -= (size_t)n;
	}
	return 0;
}

static int read_line(int fd, char* line, size_t capacity){
	size_t used = 0;
	while(used < capacity - 1){
		char byte;
		ssize_t n = recv(fd, &byte, 1, 0);
		if(n < 0 && errno == EINTR){
			continue;
		}
		if(n <= 0 || byte == '\0'){
			fprintf(stderr, "Connection ended or response is invalid\n");
			return -1;
		}
		line[used++] = byte;
		if(byte == '\n'){
			line[used] = '\0';
			return 0;
		}
	}
	line[used] = '\0';
	return 1; // A long graph line continues in the next chunk.
}

// 0: success, 1: rejected command, -1: connection/protocol error.
static int execute_command(int fd, const char* command){
	if(strlen(command) > MAX_COMMAND || strchr(command, '\n') != NULL){
		fprintf(stderr, "Expected one command of at most %d characters\n", MAX_COMMAND);
		return 1;
	}
	if(send_all(fd, command, strlen(command)) != 0 || send_all(fd, "\n", 1) != 0){
		fprintf(stderr, "Failed to send command\n");
		return -1;
	}

	char response[1024];
	if(read_line(fd, response, sizeof(response)) != 0){
		return -1;
	}
	if(strcmp(response, "GRAPH\n") == 0){
		bool line_start = true;
		while(1){
			int result = read_line(fd, response, sizeof(response));
			if(result < 0){
				return -1;
			}
			if(line_start && result == 0 && strcmp(response, "END\n") == 0){
				return 0;
			}
			fputs(response, stdout);
			line_start = result == 0;
		}
	}

	fputs(response, stdout);
	return strcmp(response, "ERROR\n") == 0 ? 1 : 0;
}

static int interactive(int fd){
	bool terminal = isatty(STDIN_FILENO);
	if(terminal){
		puts("Commands: PING, ADD_NODE id, ADD_EDGE from to weight, GET_NODE id, PRINT, QUIT");
	}

	char command[MAX_COMMAND + 2];
	while(1){
		if(terminal){
			fputs("route> ", stdout);
			fflush(stdout);
		}
		if(fgets(command, sizeof(command), stdin) == NULL){
			return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
		}

		char* newline = strchr(command, '\n');
		if(newline != NULL){
			*newline = '\0';
		}else if(strlen(command) > MAX_COMMAND){
			int byte;
			while((byte = getchar()) != '\n' && byte != EOF){
			}
			fprintf(stderr, "Command too long\n");
			continue;
		}

		if(strcmp(command, "QUIT") == 0){
			return EXIT_SUCCESS;
		}
		if(*command != '\0' && execute_command(fd, command) < 0){
			return EXIT_FAILURE;
		}
	}
}

int main(int argc, char** argv){
	if(argc > 2){
		fprintf(stderr, "Usage: %s [\"COMMAND\"]\n", argv[0]);
		return EXIT_FAILURE;
	}
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
		perror("signal");
		return EXIT_FAILURE;
	}

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0){
		perror("socket");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server = {0};
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	int status = EXIT_FAILURE;

	if(inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) != 1){
		fprintf(stderr, "Invalid server address\n");
		goto cleanup;
	}
	if(connect(fd, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("connect");
		goto cleanup;
	}

	if(argc == 2){
		status = execute_command(fd, argv[1]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
	}else{
		status = interactive(fd);
	}

cleanup:
	close(fd);
	return status;
}
