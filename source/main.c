/**
 * @file main.c
 * @brief Demonstration and test program for the fixed-size memory pool allocator.
 *
 * This program tests the allocator's allocate() and deallocate() functions by:
 *  - Allocating multiple blocks of varying sizes
 *  - Handling allocation failures
 *  - Using allocated memory
 *  - Freeing and reusing freed space
 *  - Performing a large allocation after freeing all memory
 *  - Attempting an allocation that should fail due to insufficient space
 */

#include <stdio.h>
#include "allocator.h"

/**
 * @brief Entry point of the demonstration program.
 *
 * Performs a series of allocations, deallocations, and re-allocations to verify
 * that the custom allocator behaves as expected. Finally, attempts to allocate
 * a large block (100 KB) after freeing all memory, and then tries another
 * allocation that should fail.
 *
 * @return 0 on successful execution.
 */
int main(void) {
    printf("=== Memory Allocator Test ===\n");

    /* 1. Allocate blocks of different sizes */
    int* a = allocate(128);
    printf("Allocating 128 bytes... %s\n", a ? "Success" : "Failed");

    int* b = allocate(1024);
    printf("Allocating 1024 bytes... %s\n", b ? "Success" : "Failed");

    int* c = allocate(4096);
    printf("Allocating 4096 bytes... %s\n", c ? "Success" : "Failed");

    /* 2. Use allocated memory */
    if (a) {
        a[0] = 42;
        printf("First value in 'a' set to %d\n", a[0]);
    }

    /* 3. Free one block and reallocate */
    printf("Freeing 1024 bytes block...\n");
    deallocate(b);

    b = allocate(512);
    printf("Allocating 512 bytes... %s\n", b ? "Success" : "Failed");

    /* 4. Free all remaining allocations */
    printf("Freeing all memory...\n");
    deallocate(a);
    deallocate(b);
    deallocate(c);

    /* 5. Allocate 100 KB after everything is freed */
    printf("Allocating 100 KB (102400 bytes)...\n");
    void* big_block = allocate(102400);
    printf("100 KB allocation %s\n", big_block ? "Success" : "Failed");

    /* 6. Attempt to allocate 512 bytes (should fail while big block is allocated) */
    void* fail_block = allocate(512);
    printf("Attempting 512 bytes allocation after big block... %s (expected: Failed)\n",
           fail_block ? "Success" : "Failed");

    if (fail_block) {
        deallocate(fail_block);
    }

    /* 7. Free the big block */
    if (big_block) {
        deallocate(big_block);
        printf("Freed 100 KB block.\n");
    }

    printf("=== Test Complete ===\n");
    return 0;
}
