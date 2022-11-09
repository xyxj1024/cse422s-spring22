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

int main(int argc, char **argv)
{
	int rc;

	if (argc < 2) {
		fprintf(stderr, "Usage: client <message>\n");
		exit(1);
	}

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("Could not create socket");
		exit(1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

	rc = connect(s, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (rc < 0) {
		perror("Could not connect to server");
		exit(1);
	}

	size_t len = strlen(argv[1]);
	char *msg = malloc(len + 3);
	strcpy(msg, argv[1]);
	strcat(msg, "\r\n");

	write(s, msg, strlen(msg) + 1);

	close(s);

	return 0;
}
