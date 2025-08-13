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

static void ensure_node_pool(void) {
    if (node_pool != NULL) return;
    if (full_taken) return;
    if (NODE_POOL_BYTES >= TOTAL_MEMORY) return;

    node_pool = (alloc_node_t*)(void*)g_mem.raw;

    for (uint32_t i = 0; i < MAX_NODES; ++i) {
        node_pool[i].offset = 0;
        node_pool[i].size   = 0;
        node_pool[i].next   = -1;
    }
    head_index = -1;
}

static int32_t node_slot_alloc(void) {
    for (uint32_t i = 0; i < MAX_NODES; ++i) {
        if (node_pool[i].size == 0) return (int32_t)i;
    }
    return -1;
}

static void list_insert_sorted(int32_t idx) {
    if (head_index == -1 || node_pool[idx].offset < node_pool[head_index].offset) {
        node_pool[idx].next = head_index;
        head_index = idx;
        return;
    }
    int32_t prev = head_index;
    while (node_pool[prev].next != -1 &&
           node_pool[node_pool[prev].next].offset < node_pool[idx].offset) {
        prev = node_pool[prev].next;
    }
    node_pool[idx].next = node_pool[prev].next;
    node_pool[prev].next = idx;
}

static int32_t list_remove_by_offset(uint32_t off) {
    int32_t prev = -1;
    int32_t cur = head_index;
    while (cur != -1) {
        if (node_pool[cur].offset == off) {
            if (prev == -1) head_index = node_pool[cur].next;
            else node_pool[prev].next = node_pool[cur].next;
            node_pool[cur].next = -1;
            return cur;
        }
        prev = cur;
        cur = node_pool[cur].next;
    }
    return -1;
}

static void try_uncarve_when_empty(void) {
    if (head_index != -1) return;
    node_pool = NULL;
}