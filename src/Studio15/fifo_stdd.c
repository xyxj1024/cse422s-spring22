#include <stdio.h>
#include <stdlib.h>

const char *fifo_file = "Integers";
unsigned int buf_len = 100;
char *buffer = NULL;

int main(int argc, char *argv[])
{
	int counter = 0;
	FILE *fp_fifo, *fp_stdd;
	fp_fifo = fopen(fifo_file, "w");
	fp_stdd = fopen(argv[1], "r");
	if (fp_fifo == NULL || fp_stdd == NULL) {
		fprintf(stderr, "Error: fopen\n");
		exit(EXIT_FAILURE);
	}

	buffer = (char *)malloc(buf_len * sizeof(char));
	while (getline(&buffer, &buf_len, fp_stdd) != -1) {
		if (fprintf(fp_fifo, "%s\n", buffer) > 0) {
			printf("Successfully insert line %d of size %d.\n", ++counter, buf_len);
		}
		buffer = realloc(buffer, buf_len * sizeof(char));
	}
	
	free(buffer);

	fclose(fp_fifo);
	fclose(fp_stdd);
	
	return EXIT_SUCCESS;
}
