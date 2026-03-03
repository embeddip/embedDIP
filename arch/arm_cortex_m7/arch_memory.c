/**
 * @file arch_memory.c
 * @brief ARM Cortex-M7 memory management implementation
 *
 * This implements a simple first-fit memory allocator for embedded systems.
 * Memory pool base address and size come from board configuration.
 *
 * Features:
 * - First-fit allocation algorithm
 * - Block coalescing on free
 * - 4-byte alignment
 * - Low fragmentation
 *
 * @note Memory addresses are defined by board layer, not hardcoded here
 * @note Works on any ARM Cortex-M7 board with external or internal RAM
 */

#include "arch/arch.h"
#include "arch_config.h"

#ifdef ARCH_ARM_CORTEX_M7

#include "board_config.h"  // Gets BOARD_MEMORY_POOL_BASE and SIZE
#include <stdint.h>
#include <string.h>

// ============================================================================
// Memory Block Structure
// ============================================================================

/**
 * @brief Memory block header
 *
 * Each allocated block has a header containing size and free status.
 * The header is placed immediately before the user data.
 */
typedef struct MemoryBlock {
    size_t size;                    ///< Size of data portion (excluding header)
    struct MemoryBlock* next;       ///< Next block in list
    int is_free;                    ///< 1 if free, 0 if allocated
} MemoryBlock;

// Alignment and sizes
#define ALIGN4(s) (((s) + 3) & ~3)
#define BLOCK_SIZE sizeof(MemoryBlock)

// Global state
static MemoryBlock* free_list = NULL;
static int initialized = 0;

// ============================================================================
// Memory Pool Configuration (from board)
// ============================================================================

// Memory pool base and size come from board configuration:
// - BOARD_MEMORY_POOL_BASE: Starting address
// - BOARD_MEMORY_POOL_SIZE: Total size in bytes
//
// Example board configs:
// - STM32F746G-Discovery: 8MB SDRAM at 0xC0080000 (after framebuffer)
// - STM32F767ZI-Nucleo:   512KB internal SRAM at 0x20000000
// - STM32H743ZI-Nucleo:   16MB SDRAM at 0xD0000000

// ============================================================================
// Public API Implementation
// ============================================================================

void arch_memory_init(void) {
    if (initialized) {
        return;  // Already initialized
    }

    // Get memory pool configuration from board
    uint8_t* pool_base = (uint8_t*)BOARD_MEMORY_POOL_BASE;
    size_t pool_size = BOARD_MEMORY_POOL_SIZE;

    // Validate configuration
    if (pool_size < BLOCK_SIZE + 64) {
        // Pool too small to be useful
        return;
    }

    // Initialize the free list with one large free block
    free_list = (MemoryBlock*)pool_base;
    free_list->size = pool_size - BLOCK_SIZE;
    free_list->next = NULL;
    free_list->is_free = 1;

    initialized = 1;
}

void* arch_memory_alloc(size_t size) {
    if (!initialized) {
        arch_memory_init();
    }

    if (size == 0) {
        return NULL;
    }

    // Align size to 4 bytes
    size = ALIGN4(size);

    MemoryBlock* curr = free_list;
    uint8_t* pool_base = (uint8_t*)BOARD_MEMORY_POOL_BASE;
    size_t pool_size = BOARD_MEMORY_POOL_SIZE;
    uintptr_t pool_end = (uintptr_t)pool_base + pool_size;

    // First-fit search
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            // Found a suitable block

            // Check if we can split this block
            uintptr_t curr_addr = (uintptr_t)curr;
            uintptr_t next_block_addr = curr_addr + BLOCK_SIZE + size;

            if (curr->size >= size + BLOCK_SIZE + 32 &&  // Enough space for split
                next_block_addr + BLOCK_SIZE < pool_end)   // Within pool bounds
            {
                // Split the block
                MemoryBlock* new_block = (MemoryBlock*)next_block_addr;
                new_block->size = curr->size - size - BLOCK_SIZE;
                new_block->next = curr->next;
                new_block->is_free = 1;

                curr->next = new_block;
                curr->size = size;
            }

            // Mark block as allocated
            curr->is_free = 0;

            // Return pointer to data (after header)
            return (void*)((uint8_t*)curr + BLOCK_SIZE);
        }

        curr = curr->next;
    }

    // No suitable block found
    return NULL;
}

void arch_memory_free(void* ptr) {
    if (!ptr) {
        return;  // NULL pointer is safe to free
    }

    // Validate pointer is within memory pool
    uintptr_t pool_start = (uintptr_t)BOARD_MEMORY_POOL_BASE;
    uintptr_t pool_end = pool_start + BOARD_MEMORY_POOL_SIZE;
    uintptr_t addr = (uintptr_t)ptr;

    if (addr < pool_start || addr >= pool_end) {
        // Pointer outside memory pool - ignore
        return;
    }

    if (!initialized) {
        arch_memory_init();
    }

    // Get block header (immediately before user data)
    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - BLOCK_SIZE);

    // Mark as free
    block->is_free = 1;

    // Coalesce adjacent free blocks to reduce fragmentation
    MemoryBlock* curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            // Merge curr and curr->next
            curr->size += BLOCK_SIZE + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void* arch_memory_realloc(void* ptr, size_t new_size) {
    if (!ptr) {
        // NULL pointer - behave like malloc
        return arch_memory_alloc(new_size);
    }

    if (new_size == 0) {
        // Zero size - behave like free
        arch_memory_free(ptr);
        return NULL;
    }

    if (!initialized) {
        arch_memory_init();
    }

    // Get current block header
    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - BLOCK_SIZE);

    // Align new size
    new_size = ALIGN4(new_size);

    // If current block is large enough, return same pointer
    if (block->size >= new_size) {
        return ptr;
    }

    // Need to allocate new block
    void* new_ptr = arch_memory_alloc(new_size);
    if (!new_ptr) {
        // Allocation failed - original block unchanged
        return NULL;
    }

    // Copy data from old to new (copy only old size)
    memcpy(new_ptr, ptr, block->size);

    // Free old block
    arch_memory_free(ptr);

    return new_ptr;
}

#endif // ARCH_ARM_CORTEX_M7
