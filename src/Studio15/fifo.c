#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
	char *my_file = "LochLomond";
	int c;
	FILE *file_stream = NULL;

	if (mkfifo(my_file, S_IRUSR | S_IWUSR) == -1) {
		fprintf(stderr, "Error: mkfifo %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	if ((file_stream = fopen(my_file, "r")) == NULL) {
		fprintf(stderr, "Error: fopen fifo\n");
		exit(EXIT_FAILURE);
	}
	
	while ((c = fgetc(file_stream)) != EOF) {
		fprintf(stdout, "%c", c);
	}
	
	fclose(file_stream);

	return EXIT_SUCCESS;
}
