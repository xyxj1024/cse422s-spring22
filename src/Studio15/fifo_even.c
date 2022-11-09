#include <stdio.h>
#include <stdlib.h>

const char *fifo_name = "Integers";

int main()
{
	int d;
	FILE *even_int;
	even_int = fopen(fifo_name, "w");
	if (even_int == NULL) {
		fprintf(stderr, "Error: fopen fifo\n");
		exit(EXIT_FAILURE);
	}
	while (fscanf(stdin, "%d", &d) == 1) {
		if (d % 2 != 0) continue;
		if (fprintf(even_int, "Original: %d; New: %d.\n", d, 2*d) > 0) {
			printf("Successfully insert %d.\n", d);
		}
	}
	fclose(even_int);
	
	return EXIT_SUCCESS;
}
