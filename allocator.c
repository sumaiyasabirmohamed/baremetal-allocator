#include "allocator.h"
#include <stdint.h>
#include <stddef.h>

#define TOTAL_MEMORY   (100u * 1024u)  /* 100 KB */
#define MAX_NODES      96
#define NODE_POOL_BYTES  ((uint32_t)(MAX_NODES * (uint32_t)sizeof(alloc_node_t)))

typedef struct {
    uint32_t offset;
    uint32_t size;
    int32_t  next;
} alloc_node_t;

typedef union {
    max_align_t align;
    uint8_t     raw[TOTAL_MEMORY];
} ram_block_t;

static ram_block_t g_mem;
static alloc_node_t *node_pool = NULL;
static int32_t head_index = -1;
static uint8_t full_taken = 0;
