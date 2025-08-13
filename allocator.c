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

int *allocate(int size) {
    if (size <= 0) return NULL;
    uint32_t req = (uint32_t)size;

    if (req == TOTAL_MEMORY) {
        if (full_taken || node_pool != NULL || head_index != -1) return NULL;
        full_taken = 1;
        return (int*)(void*)&g_mem.raw[0];
    }

    if (full_taken) return NULL;

    ensure_node_pool();
    if (node_pool == NULL) return NULL;

    const uint32_t USABLE_BASE  = NODE_POOL_BYTES;
    const uint32_t USABLE_LIMIT = TOTAL_MEMORY; /* exclusive */

    /* Case 1: no allocations yet */
    if (head_index == -1) {
        if (USABLE_BASE + req <= USABLE_LIMIT) {
            int32_t idx = node_slot_alloc();
            if (idx < 0) return NULL;
            node_pool[idx].offset = USABLE_BASE;
            node_pool[idx].size   = req;
            list_insert_sorted(idx);
            return (int*)(void*)(&g_mem.raw[node_pool[idx].offset]);
        }
        return NULL;
    }

    /* Case 2: gap before first allocation */
    {
        uint32_t first_off = node_pool[head_index].offset;
        if (first_off >= USABLE_BASE + req) {
            int32_t idx = node_slot_alloc();
            if (idx < 0) return NULL;
            node_pool[idx].offset = USABLE_BASE;
            node_pool[idx].size   = req;
            list_insert_sorted(idx);
            return (int*)(void*)(&g_mem.raw[node_pool[idx].offset]);
        }
    }

    /* Case 3: gaps between existing blocks */
    for (int32_t cur = head_index; cur != -1; cur = node_pool[cur].next) {
        int32_t nxt = node_pool[cur].next;
        uint32_t gap_start = node_pool[cur].offset + node_pool[cur].size;
        uint32_t gap_end   = (nxt == -1) ? USABLE_LIMIT : node_pool[nxt].offset;
        if (gap_end > gap_start && (gap_end - gap_start) >= req) {
            int32_t idx = node_slot_alloc();
            if (idx < 0) return NULL;
            node_pool[idx].offset = gap_start;
            node_pool[idx].size   = req;
            list_insert_sorted(idx);
            return (int*)(void*)(&g_mem.raw[node_pool[idx].offset]);
        }
    }

    return NULL;
}

void deallocate(int *ptr) {
    if (!ptr) return;
    uint8_t *p = (uint8_t*)(void*)ptr;

    if (p < g_mem.raw || p >= (g_mem.raw + TOTAL_MEMORY)) return;

    if (full_taken && p == &g_mem.raw[0]) {
        full_taken = 0;
        return;
    }

    if (node_pool == NULL) return;

    uint32_t off = (uint32_t)(p - g_mem.raw);
    int32_t idx = list_remove_by_offset(off);
    if (idx < 0) return;

    node_pool[idx].size   = 0;
    node_pool[idx].offset = 0;

    if (head_index == -1) {
        try_uncarve_when_empty();
    }
}