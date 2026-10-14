#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main(void){
	printf("Network Route Engine Client\n");

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(sock_fd == -1){
		perror("socket");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server = {0};

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);

	if(inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) != 1){
		fprintf(stderr, "Invalid IP address\n");
		close(sock_fd);
		return EXIT_FAILURE;
	}

	if(connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1){
		perror("connect");
		close(sock_fd);
		return EXIT_FAILURE; 
	}

	printf("Connected to server!\n");

	const char *message = "HELLO";
	size_t length = strlen(message);
	size_t sent = 0;

	while(sent < length){
		ssize_t n = send(sock_fd, message + sent, length - sent, 0);

		if(n <= 0){
			if(n < 0){
				perror("send");
			}else{
				fprintf(stderr, "sent returned 0\n");
			}

			close(sock_fd);
			return EXIT_FAILURE;
		}

		sent += (size_t)n;
	}

	printf("Sent: %s\n", message);


	char buffer[BUFFER_SIZE];

	ssize_t received = recv(sock_fd, buffer, sizeof(buffer)-1, 0);

	if(received <= 0){
		if(received < 0){
			perror("recv");
			close(sock_fd);
			return EXIT_FAILURE;
		}

		if(received == 0){
			printf("Server close connection\n");
			close(sock_fd);
			return EXIT_SUCCESS;
		}
	}

	buffer[received] = '\0';

	printf("Received: %s\n", buffer);

	close(sock_fd);

	return EXIT_SUCCESS;

}
