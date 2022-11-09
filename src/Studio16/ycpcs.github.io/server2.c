// This is like server.c, but uses fdopen to allow communication
// via stdio functions rather than Unix system calls

#include <unistd.h>
#include <stdio.h> // for perror()
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <netinet/in.h> // for struct sockaddr_in
#include <arpa/inet.h> // for inet_pton

#include <string.h>

#define PORT 40002
#define BUFSIZE 2000

int main(void)
{
	int server_sock_fd;
	int rc;
	char buf[BUFSIZE];
	int done = 0;

	server_sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock_fd < 0) {
		perror("Couldn't create socket");
		exit(1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);

	rc = bind(
		server_sock_fd,
		(struct sockaddr *) &server_addr,
		sizeof(server_addr));
	if (rc < 0) {
		perror("Couldn't bind server socket");
		exit(1);
	}

	rc = listen(server_sock_fd, 5);
	if (rc < 0) {
		perror("Couldn't listen on server socket");
		exit(1);
	}

	while (!done) {
		int client_socket_fd;
		struct sockaddr_in client_addr;
		socklen_t client_addr_size = sizeof(client_addr);

		client_socket_fd = accept(
			server_sock_fd,
			(struct sockaddr*) &client_addr,
			&client_addr_size);
		if (client_socket_fd < 0) {
			perror("Couldn't accept connection");
			exit(1);
		}

		// Wrap the socket file descriptor using a FILE*,
		// for reading only.
		FILE *client_socket_fh = fdopen(client_socket_fd, "r");

		// Read one line from the client
		fgets(buf, BUFSIZE, client_socket_fh);

		// Close the client socket
		fclose(client_socket_fh);

		// Write the received message to stdout
		printf("%s", buf);

		if (strcmp(buf, "quit\r\n") == 0) {
			done = 1;
		}
	}

	close(server_sock_fd);

	return 0;
}
