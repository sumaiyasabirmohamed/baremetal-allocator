/**
 * @file main.c
 * @brief Demonstration program for the fixed-size memory pool allocator.
 *
 * This program exercises the allocator's allocate() and deallocate() functions
 * by performing various memory allocation and deallocation operations.
 * It demonstrates:
 *  - Allocating multiple blocks of varying sizes
 *  - Handling allocation failures
 *  - Using allocated memory
 *  - Freeing memory and reusing freed space
 */

#include <stdio.h>
#include "allocator.h"

/**
 * @brief Entry point of the demonstration program.
 *
 * Allocates several memory blocks, manipulates them, frees some blocks,
 * then reallocates new blocks to show memory reuse.
 *
 * @return 0 on successful execution.
 */
int main(void) {
    int* mem[5] = { NULL }; /**< Array to store pointers to allocated blocks. */

    /* Allocate 128 bytes */
    mem[0] = allocate(128);
    if (!mem[0]) {
        printf("Failed to allocate 128 bytes\n");
    }

    /* Allocate 1024 bytes */
    mem[1] = allocate(1024);
    if (!mem[1]) {
        printf("Failed to allocate 1024 bytes\n");
    }

    /* Allocate 4096 bytes */
    mem[2] = allocate(4096);
    if (!mem[2]) {
        printf("Failed to allocate 4096 bytes\n");
    }

    /* Example usage: store a value in the first allocated block */
    if (mem[0]) {
        mem[0][0] = 42;
    }

    /* Free the 1024-byte block */
    deallocate(mem[1]);
    mem[1] = NULL;

    /* Allocate a smaller block (512 bytes) to test reuse of freed space */
    mem[1] = allocate(512);
    if (!mem[1]) {
        printf("Failed to allocate 512 bytes\n");
    }

    /* Clean up all remaining allocations */
    for (int i = 0; i < 3; i++) {
        if (mem[i]) {
            deallocate(mem[i]);
            mem[i] = NULL;
        }
    }

    return 0;
}
