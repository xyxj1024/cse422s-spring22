/* sock_server.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SERVER_PATH "/tmp/studio16"

#define BACKLOG 10

#define handle_error(msg, fd) \
        do { perror(msg); close(fd); exit(EXIT_FAILURE); } while(0)

#define FALSE 0

int main()
{
    int server_fd = -1, client_fd = -1, rc = -1;
    int option = 1;
    unsigned int val = 0;
    struct sockaddr_un server_addr;
    FILE* fp = NULL;

    do
    {

        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd < 0) {
            handle_error("socket()", server_fd);
        }
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVER_PATH);

        rc = bind(server_fd, (struct sockaddr *)&server_addr, SUN_LEN(&server_addr));
        if (rc < 0) {
            handle_error("bind()", server_fd);
        }

        rc = listen(server_fd, BACKLOG);
        if (rc < 0) {
            handle_error("listen()", server_fd);
        }

        fprintf(stdout, "Ready for client connect().\n");

        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            handle_error("accept()", client_fd);
        }
        fprintf(stdout, "Accept connection.\n");

        fp = fdopen(dup(client_fd), "r");   
        if (!fp) {
            handle_error("fdopen()", client_fd);
        }

        while (fscanf(fp, "%u", &val) == 1) {
            fprintf(stdout, "Read in %u.\n", val);
        }

        fclose(fp);

    } while (FALSE);

    if (server_fd != -1) {
        close(server_fd);
    }

    if (client_fd != -1) {
        close(client_fd);
    }

    unlink(SERVER_PATH);

    return EXIT_SUCCESS;
}
