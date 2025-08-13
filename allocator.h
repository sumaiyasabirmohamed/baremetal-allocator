#ifndef ALLOCATOR_H
#define ALLOCATOR_H

/**
 * @file allocator.h
 * @brief Simple fixed-size memory pool allocator for bare-metal environment.
 *
 * This allocator manages a single, statically allocated, contiguous memory pool.
 * It provides a minimal public API for allocating and freeing memory without
 * relying on the C standard library (`malloc`/`free`).
 *
 */

/**
 * @brief Allocates a block of memory from the static memory pool.
 *
 * @param size  Number of bytes to allocate (must be > 0)
 * @return Pointer to allocated memory (aligned to sizeof(int)), or NULL if
 *         allocation fails (insufficient space or invalid request).
 *
 */
int *allocate(int size);

/**
 * @brief Frees a previously allocated memory block.
 *
 * @param ptr  Pointer returned by allocate().
 *
 */
void deallocate(int *ptr);

#endif /* ALLOCATOR_H */