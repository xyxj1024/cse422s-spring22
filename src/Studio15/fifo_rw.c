#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
	char *my_file = "Integers";
	int c;
	FILE *file_stream_r = NULL, *file_stream_w = NULL;

	if (mkfifo(my_file, S_IRUSR | S_IWUSR) == -1) {
		fprintf(stderr, "Error: mkfifo %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	if ((file_stream_r = fopen(my_file, "r")) == NULL) {
		fprintf(stderr, "Error: fopen fifo for read\n");
		exit(EXIT_FAILURE);
	}
	if ((file_stream_w = fopen(my_file, "w")) == NULL) {
		fprintf(stderr, "Error: fopen fifo for write\n");
		exit(EXIT_FAILURE);
	}
	
	while (fscanf(file_stream_r, "%d", &c) == 1) {
		fprintf(stdout, "Original: %d; New: %d.\n", c, 2*c);
	}
	
	fclose(file_stream_r);
	fclose(file_stream_w);

	return EXIT_SUCCESS;
}
