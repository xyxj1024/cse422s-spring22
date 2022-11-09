// Like server3.c, but uses threads to allow multiple clients
// to connect at the same time.

// Running the server:
//    ./server4 <port>
// where <port> is the port number.

// Use telnet as the client:
//    telnet <port>

#include <unistd.h>
#include <stdio.h> // for perror()
#include <stdlib.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <netinet/in.h> // for struct sockaddr_in
#include <arpa/inet.h> // for inet_pton

#include <string.h>

#define BUFSIZE 2000

// Server data structure: keeps track of how many workers are
// active, and whether a shutdown has been requested
typedef struct {
	int port;
	int num_workers;
	int shutdown_requested;

	pthread_mutex_t lock;
	pthread_cond_t cond;
} Server;

// Data used by a worker that chats with a specific client
typedef struct {
	Server *s;
	int client_fd;
} Worker;

// Server task - waits for incoming connections, and spawns workers
void *server(void *arg);

// Worker task - chats with clients
void *worker(void *arg);

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: ./server4 <port>\n");
		exit(1);
	}

	Server *s = malloc(sizeof(Server));

	// Initialize Server structure
	s->port = atoi(argv[1]);
	s->num_workers = 0;
	s->shutdown_requested = 0;
	pthread_mutex_init(&s->lock, NULL);
	pthread_cond_init(&s->cond, NULL);

	// Start server thread
	pthread_t thr;
	pthread_create(&thr, NULL, &server, s);

	// Wait for shutdown and for workers to finish
	pthread_mutex_lock(&s->lock);
	while (!s->shutdown_requested || s->num_workers > 0) {
		pthread_cond_wait(&s->cond, &s->lock);
	}
	pthread_mutex_unlock(&s->lock);

	// Join server thread
	pthread_join(thr, NULL);

	// Free Server structure
	free(s);

	return 0;
}

void *server(void *arg)
{
	Server *s = arg;

	int server_sock_fd;

	server_sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock_fd < 0) {
		perror("Couldn't create socket");
		exit(1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(s->port);
	inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);

	int rc = bind(
		server_sock_fd,
		(struct sockaddr *) &server_addr,
		sizeof(server_addr));
	if (rc < 0) {
		perror("Couldn't bind server socket");
		exit(1);
	}

	// Make the server socket nonblocking
	fcntl(server_sock_fd, F_SETFL, O_NONBLOCK);

	// Create a fd_set with the server socket file descriptor
	// (so we can use select to to a timed wait for incoming
	// connections.)
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(server_sock_fd, &fds);

	rc = listen(server_sock_fd, 5);
	if (rc < 0) {
		perror("Couldn't listen on server socket");
		exit(1);
	}

	int done = 0;
	while (!done) {
		// Use select to see if there are any incoming connections
		// ready to accept

		// Set a 1-second timeout
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		// We want to wait for just the server socket
		fd_set readyfds = fds;

		// Preemptively increase the worker count
		pthread_mutex_lock(&s->lock);
		s->num_workers++;
		pthread_mutex_unlock(&s->lock);

		// Wait until either a connection is received, or the timeout expires
		int rc = select(server_sock_fd+1, &readyfds, NULL, NULL, &timeout);

		if (rc == 1) {
			// The server socket file descriptor became ready,
			// so we can accept a connection without blocking
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

			// Create Worker data structure (which tells the worker
			// thread what it's client socket fd is, and
			// gives it access to the Server struct)
			Worker *w = malloc(sizeof(Worker));
			w->s = s;
			w->client_fd = client_socket_fd;

			// Start a worker thread (detached, since no other thread
			// will wait for it to complete)
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

			pthread_t thr;
			pthread_create(&thr, &attr, &worker, w);
		} else {
			// The select timed out.

			// Decrement the worker count (since worker wasn't
			// actually started.)
			pthread_mutex_lock(&s->lock);
			s->num_workers--;
			pthread_cond_broadcast(&s->cond);
			pthread_mutex_unlock(&s->lock);

			// See if a shutdown was requested, and if so, stop
			// waiting for connections.
			pthread_mutex_lock(&s->lock);
			if (s->shutdown_requested) {
				done = 1;
			}
			pthread_mutex_unlock(&s->lock);
		}
	}

	close(server_sock_fd);

	return NULL;
}

void *worker(void *arg)
{
	Worker *w = arg;
	Server *s = w->s;

	char buf[BUFSIZE];
	int read_fd = w->client_fd;
	int write_fd = dup(w->client_fd); // duplicate the file descriptor

	// Wrap the socket file descriptor using FILE* file handles,
	// for both reading and writing.
	FILE *read_fh = fdopen(read_fd, "r");
	FILE *write_fh = fdopen(write_fd, "w");

	int done = 0;

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

			// A shutdown was requested
			pthread_mutex_lock(&s->lock);
			s->shutdown_requested = 1;
			pthread_cond_broadcast(&s->cond);
			pthread_mutex_unlock(&s->lock);
		}
	}

	// Close the file handles wrapping the client socket
	fclose(read_fh);
	fclose(write_fh);

	// Free Worker struct
	free(w);

	// Update worker count
	pthread_mutex_lock(&s->lock);
	s->num_workers--;
	pthread_cond_broadcast(&s->cond);
	pthread_mutex_unlock(&s->lock);

	return NULL;
}
