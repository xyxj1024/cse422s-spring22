// A server that does two-way communication with the client.
// The protocol is that the server reads a line of text,
// then echoes is back to the client.  If the server receives
// the line "quit", it terminates the connection with the
// client (and resumes listening for connections.)  If the
// server receives "shutdown", it terminates the connection
// with the client, and shuts down.

// Running the server:
//    ./server3 <port>
// where <port> is the port number.

// Use telnet as the client:
//    telnet <port>

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

#define BUFSIZE 2000

// Chat with a client.  Returns 0 if the client sent a "shutdown"
// command, 1 otherwise.
int chat_with_client(int client_fd);

int main(int argc, char **argv)
{
	int server_sock_fd;
	int rc;
	int done = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: ./server3 <port>\n");
		exit(1);
	}
	int port = atoi(argv[1]);

	server_sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock_fd < 0) {
		perror("Couldn't create socket");
		exit(1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
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

		int rc = chat_with_client(client_socket_fd);
		if (rc == 0) {
			done = 1;
		}
	}

	close(server_sock_fd);

	return 0;
}

int chat_with_client(int client_fd)
{
	char buf[BUFSIZE];
	int read_fd = client_fd;
	int write_fd = dup(client_fd); // duplicate the file descriptor

	// Wrap the socket file descriptor using FILE* file handles,
	// for both reading and writing.
	FILE *read_fh = fdopen(read_fd, "r");
	FILE *write_fh = fdopen(write_fd, "w");

	int done = 0;

	int result = 1;
	while (!done) {
		// Read one line from the client
		if (!fgets(buf, BUFSIZE, read_fh)) {
			break; // connection was interrupted?
		}

		// If a \r character is seen, change it to \n and
		// terminate the string.  (This is needed if the client
		// is telnet, since if the user types enter, telnet
		// sends \r\n.)
		char *cr = strchr(buf, '\r');
		if (cr) {
			*cr++ = '\n';
			*cr = '\0';
		}

		// Send the received message back to the client
		fprintf(write_fh, "%s", buf);
		fflush(write_fh);

		if (strcmp(buf, "quit\n") == 0) {
			done = 1;
		} else if (strcmp(buf, "shutdown\n") == 0) {
			done = 1;
			result = 0; // this will shut down the server
		}
	}

	// Close the file handles wrapping the client socket
	fclose(read_fh);
	fclose(write_fh);

	return result;
}
