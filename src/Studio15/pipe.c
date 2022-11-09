#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_NUM 10
#define MAX_LINE_LEN 100

int main()
{
	char msg[MAX_LINE_LEN];
	int i, pipefds[2];
	pid_t p;
	FILE *file_stream;

	if (pipe(pipefds) == -1) {
		fprintf(stderr, "Error: pipe()\n");
		exit(EXIT_FAILURE);
	}
	
	p = fork();
	if (p < 0) {
		fprintf(stderr, "Error: fork()\n");
		exit(EXIT_FAILURE);
	}
	else if (p > 0) { 		// Parent process
		close(pipefds[1]);	// Close writing end
		if ((file_stream = fdopen(pipefds[0], "r")) == NULL) {
			fprintf(stderr, "Error, fdopen(parent)\n");
			exit(EXIT_FAILURE);
		}

					// Receive hello from child
		if (fgets(msg, MAX_LINE_LEN, file_stream) != NULL) {
			printf("%s", msg);
		}

					// Receive messages from child
		i = 1;
		while (fgets(msg, MAX_LINE_LEN, file_stream) != NULL) {
			printf("%d. %s", i++, msg);
		}

		printf("Parent with pid %u received from child: %s", getpid(), msg);
		fclose(file_stream);
		wait(NULL); 		// Wait for child to exit
	}
	else {				// Child process
		close(pipefds[0]); 	// Close reading end
		
		if ((file_stream = fdopen(pipefds[1], "w")) == NULL) {
			fprintf(stderr, "Error: fdopen(child)\n");
			exit(EXIT_FAILURE);
		}
		
		fprintf(file_stream, "Hello from child process with pid %u!\n", getpid());
		
					// Write MAX_LINE_NUM lines to the file_stream
		for (i = 0; i < MAX_LINE_NUM; i++) {
			fprintf(file_stream, "%s\n", "We are walking in the air...");
		}
		fclose(file_stream);
	}

	return EXIT_SUCCESS;
}
