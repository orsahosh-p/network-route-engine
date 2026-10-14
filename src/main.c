#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVER_PORT 8080

int main(void){
	printf("Network Route Engine Server\n");

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(server_fd == -1){
		perror("socket");
		return EXIT_FAILURE;
	}

	struct sockaddr_in address = {0};

	address.sin_family = AF_INET;
	address.sin_port = htons(SERVER_PORT);

	if(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1){
		fprintf(stderr, "Invalid IP adddress\n");
		close(server_fd);
		return EXIT_FAILURE;
	}

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1){
		perror("bind");
		close(server_fd);
		return EXIT_FAILURE;
	}

	if(listen(server_fd, 5) == -1){
		perror("listen");
		close(server_fd);
		return EXIT_FAILURE;
	}

	printf("Server listening on 127.0.0.1:%d\n", SERVER_PORT);
	fflush(stdout);

	int client_fd = accept(server_fd, NULL, NULL);

	if(client_fd == -1){
		perror("accept");
		close(server_fd);
		return EXIT_FAILURE;
	}

	printf("Client connected!\n");

	char buffer[1024];

	ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	if(received <= 0){
		if(received < 0){
			perror("recv");
		}else{
			printf("Client close connection!\n");
		}

		close(client_fd);
		close(server_fd);
		return EXIT_FAILURE;
	}

	buffer[received] = '\0';

	printf("Received: %s\n", buffer);

	size_t sent = 0;
	size_t total = (size_t)received;

	while(sent < total){
		ssize_t n = send(client_fd, buffer+sent, total-sent, 0);

		if(n <= 0){
			if(n < 0){
				perror("send");
			}else{
				fprintf(stderr, "send returned 0\n");
				close(client_fd);
				close(server_fd);
				return EXIT_SUCCESS;
			}

			close(client_fd);
			close(server_fd);

			return EXIT_FAILURE;
		}

		sent += (size_t)n;
	}

	close(client_fd);
	close(server_fd);

	return EXIT_SUCCESS;
}
