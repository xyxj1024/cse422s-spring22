/* do_fork.c from Smith and Maguire (1988) */
#include <errno.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef NBPC
#define PAGE_SIZE 2048
#else
#define PAGE_SIZE NBPC
#endif

int main(int argc, char *argv[])
{
    int count = 0, heap_size = 0, pid, status;
    double write_fraction = 0.0, write_count = 0.0, write_size = 0.0;
    register char *ptr = NULL;

    if (argc > 1) {
        count = atoi(argv[1]);
        if (argc > 2) {
            heap_size = atoi(argv[2]);
            if ((ptr = malloc(heap_size)) == NULL) {
                perror("Insufficient memory available. Exiting.\n");
            }
            if (argc > 3) {
                write_fraction = atof(argv[3]);
                if ((write_fraction < 0.0) || write_fraction > 1.0) {
                    perror("0.0 <= write_fraction <= 1.0. Exiting.\n");
                }
                write_size = write_fraction * (double)heap_size;
            }
        }
    }

    while (count > 0) {
        switch (pid = fork()) {
        case -1:
            if (errno == EAGAIN)
                wait(&status);
            break;
        
        case 0:
            while (write_count < write_size) {
                *ptr = ' ';
                ptr = &ptr[PAGE_SIZE];
                write_count += (double)PAGE_SIZE;
            }
            exit(0);
        
        default:
            count -= 1;
        }
    }

    return 0;
}
