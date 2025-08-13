/**
 * @file allocator.c
 * @brief Implementation of a simple fixed-size memory pool allocator for
 *        bare-metal environments.
 *
 * This file contains the definitions for memory allocation and deallocation
 * using a statically allocated pool with optional metadata tracking.
 * It is intended for systems without a standard heap manager.
 */

#include "allocator.h"
#include <stdint.h>
#include <stddef.h>

/* ---------------------------------------------------------------------------- */
/*                           Configuration Constants                            */
/* ---------------------------------------------------------------------------- */

/**
 * @def TOTAL_MEMORY
 * @brief Total managed memory in bytes (pool + optional metadata).
 */
#define TOTAL_MEMORY   (100u * 1024u)  /**< 100 KB */

/**
 * @def MAX_NODES
 * @brief Maximum number of allocation metadata entries.
 */
#define MAX_NODES      96

/**
 * @def NODE_POOL_BYTES
 * @brief Bytes required for all metadata entries combined.
 */
#define NODE_POOL_BYTES  ((uint32_t)(MAX_NODES * (uint32_t)sizeof(alloc_node_t)))

/* ---------------------------------------------------------------------------- */
/*                              Internal Data Types                             */
/* ---------------------------------------------------------------------------- */

/**
 * @struct alloc_node_t
 * @brief Allocation tracking entry.
 *
 * @var alloc_node_t::offset
 *      Byte offset from g_mem.raw[] where this block starts.
 * @var alloc_node_t::size
 *      Block size in bytes (0 means metadata slot is unused).
 * @var alloc_node_t::next
 *      Index of the next allocated block in sorted order (-1 = end of list).
 */
typedef struct {
    uint32_t offset;
    uint32_t size;
    int32_t  next;
} alloc_node_t;

/**
 * @union ram_block_t
 * @brief Managed memory pool.
 *
 * @var ram_block_t::_align
 *      Ensures alignment for any data type.
 * @var ram_block_t::raw
 *      The actual byte storage.
 */
typedef union {
    max_align_t _align;
    uint8_t     raw[TOTAL_MEMORY];
} ram_block_t;

/* ---------------------------------------------------------------------------- */
/*                                 Internal State                               */
/* ---------------------------------------------------------------------------- */

/** Primary memory pool. */
static ram_block_t g_mem;

/** Pointer to carved metadata area (NULL if uninitialized). */
static alloc_node_t *node_pool = NULL;

/** Index of first allocated block in sorted list (-1 if empty). */
static int32_t head_index = -1;

/** Flag indicating the entire memory pool is allocated in one block. */
static uint8_t full_taken = 0;

/* ---------------------------------------------------------------------------- */
/*                                 Internal Helpers                             */
/* ---------------------------------------------------------------------------- */

/**
 * @brief Lazily initializes metadata by carving it from the start of g_mem.raw[].
 *
 * Called only when the first non-special allocation occurs.
 * If the full memory pool is already taken or there is insufficient space,
 * metadata is not initialized.
 */
static void ensure_node_pool(void) {
    if (node_pool != NULL) return;                  /* already initialized */
    if (full_taken) return;                         /* can't carve if full buffer taken */
    if (NODE_POOL_BYTES >= TOTAL_MEMORY) return;    /* not enough space */

    node_pool = (alloc_node_t*)(void*)g_mem.raw;

    /* Mark all metadata slots as unused */
    for (uint32_t i = 0; i < MAX_NODES; ++i) {
        node_pool[i].offset = 0;
        node_pool[i].size   = 0;
        node_pool[i].next   = -1;
    }
    head_index = -1;
}

/**
 * @brief Returns index of a free metadata slot.
 *
 * @return Index of a free metadata slot, or -1 if none are available.
 */
static int32_t node_slot_alloc(void) {
    for (uint32_t i = 0; i < MAX_NODES; ++i) {
        if (node_pool[i].size == 0) return (int32_t)i;
    }
    return -1;
}

/**
 * @brief Inserts a node into the linked list of allocations in offset order.
 *
 * @param idx Index of the metadata node to insert.
 */
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

/**
 * @brief Removes the block starting at a given offset from the list.
 *
 * @param off Offset of the block in bytes from g_mem.raw[].
 * @return Index of the metadata entry removed, or -1 if not found.
 */
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

/**
 * @brief Resets node_pool to NULL when all allocations are freed.
 */
static void try_uncarve_when_empty(void) {
    if (head_index != -1) return; /* still in use */
    node_pool = NULL;
}

/* ---------------------------------------------------------------------------- */
/*                              Public API Implementation                       */
/* ---------------------------------------------------------------------------- */

/**
 * @brief Allocates a block of memory from the static memory pool.
 *
 * @param size Number of bytes to allocate (must be > 0).
 * @return Pointer to allocated memory (aligned to sizeof(int)), or NULL if
 *         allocation fails (insufficient space or invalid request).
 *
 * @note If the request size equals TOTAL_MEMORY, the allocator will
 *       bypass metadata tracking and give the whole pool.
 */
int *allocate(int size) {
    if (size <= 0) return NULL;
    uint32_t req = (uint32_t)size;

    /* Special case: allocate entire pool */
    if (req == TOTAL_MEMORY) {
        if (full_taken || node_pool != NULL || head_index != -1) return NULL;
        full_taken = 1;
        return (int*)(void*)&g_mem.raw[0];
    }

    /* Pool is fully taken — no further allocations */
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

    return NULL; /* no suitable space */
}

/**
 * @brief Frees a previously allocated memory block.
 *
 * @param ptr Pointer returned by allocate().
 *
 * @note If freeing the full pool allocation, metadata tracking is not restored.
 */
void deallocate(int *ptr) {
    if (!ptr) return;
    uint8_t *p = (uint8_t*)(void*)ptr;

    if (p < g_mem.raw || p >= (g_mem.raw + TOTAL_MEMORY)) return;

    /* Special case: freeing full buffer */
    if (full_taken && p == &g_mem.raw[0]) {
        full_taken = 0;
        return;
    }

    if (node_pool == NULL) return;

    uint32_t off = (uint32_t)(p - g_mem.raw);
    int32_t idx = list_remove_by_offset(off);
    if (idx < 0) return; /* invalid */

    /* Mark slot as free */
    node_pool[idx].size   = 0;
    node_pool[idx].offset = 0;

    /* If nothing left, release metadata */
    if (head_index == -1) {
        try_uncarve_when_empty();
    }
}
