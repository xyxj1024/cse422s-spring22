/* My timed_parallel_dense_mm.c program */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

const int num_expected_args = 2;
const unsigned sqrt_of_UINT32_MAX = 65536;
const long BILLION = 1000000000L;

// http://c-faq.com/stdio/commaprint.html
#include <locale.h>
char *commaprint(unsigned long n)
{
	static int comma = '\0';
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	int i = 0;

	if(comma == '\0') {
		struct lconv *lcp = localeconv();
		if(lcp != NULL) {
			if(lcp->thousands_sep != NULL &&
				*lcp->thousands_sep != '\0')
				comma = *lcp->thousands_sep;
			else	comma = ',';
		}
	}

	*p = '\0';

	do {
		if(i%3 == 0 && i != 0)
			*--p = comma;
		*--p = '0' + n % 10;
		n /= 10;
		i++;
	} while(n != 0);

	return p;
}

int main( int argc, char* argv[] )
{
    unsigned index, row, col; // Loop indices
    unsigned matrix_size, squared_size;
    double *A, *B, *C;
    #ifdef VERIFY_PARALLEL
    double *D;
    #endif

    unsigned iterations = 1; // Default number of iterations
    unsigned i = 0;
    unsigned long min = 2147483647, max = 0, sum = 0, average = 0;
    struct timespec start, end;
    long interval;

    if ( argc < num_expected_args || argc > num_expected_args + 1 ) {
        printf("Usage: ./dense_mm <size of matrices> <number of iterations>\n");
	exit(-1);
    }

    matrix_size = atoi(argv[1]);
    if ( argc == 3 ) iterations = atoi(argv[2]);

    if ( matrix_size > sqrt_of_UINT32_MAX ) {
	printf("ERROR: Matrix size must be between zero and 65536!\n");
	exit(-1);
    }

    squared_size = matrix_size * matrix_size;
    printf("Generating matrices...\n");

    A = (double*) malloc( sizeof(double) * squared_size );
    B = (double*) malloc( sizeof(double) * squared_size );
    C = (double*) malloc( sizeof(double) * squared_size );
    #ifdef VERIFY_PARALLEL
    D = (double*) malloc( sizeof(double) * squared_size );
    #endif

    for( index = 0; index < squared_size; index++ ) {
        A[index] = (double) rand();
	B[index] = (double) rand();
	C[index] = 0.0;
	#ifdef VERIFY_PARALLEL
	D[index] = 0.0;
	#endif
    }

    printf("Multiplying matrices...\n");
    for ( i = 0; i < iterations; i++ ) {
        clock_gettime( CLOCK_MONOTONIC_RAW, &start );
        #pragma omp parallel for private(col, row, index) // Critical section
	for( col = 0; col < matrix_size; col++ ) {
            for( row = 0; row < matrix_size; row++ ) {
	        for( index = 0; index < matrix_size; index++) {
		    C[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
                }	
            }
	}
        clock_gettime( CLOCK_MONOTONIC_RAW, &end );
        interval = (end.tv_sec * BILLION - start.tv_sec * BILLION) + (end.tv_nsec - start.tv_nsec);
        sum += interval;
        if ( interval < min ) min = interval;
        if ( interval > max ) max = interval; 
    }
    average = sum / iterations;
    printf("%25s\t%15s\t%15s\n", "Statistics", "secs", "nsecs");
    printf("%25s\t%15s\t%15s\n", "Minimum", commaprint(min/BILLION), commaprint(min%BILLION));
    printf("%25s\t%15s\t%15s\n", "Maximum", commaprint(max/BILLION), commaprint(max%BILLION));
    printf("%25s\t%15s\t%15s\n", "Average", commaprint(average/BILLION), commaprint(average%BILLION));

    #ifdef VERIFY_PARALLEL
    printf("Verifying parallel matrix multiplication...\n");
    for( col = 0; col < matrix_size; col++ ) {
        for( row = 0; row < matrix_size; row++ ) {
            for( index = 0; index < matrix_size; index++) {
	        D[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
	    }	
	}
    }

    for( index = 0; index < squared_size; index++ ) 
        assert( C[index] == D[index] );
    #endif //ifdef VERIFY_PARALLEL

    printf("Multiplication done!\n");

    return 0;
}
