/**
 * @file arch_memory.c
 * @brief HOST architecture memory management (standard malloc/free)
 *
 * This file provides memory allocation for HOST architecture using
 * standard C library malloc/free functions.
 *
 * No custom memory pool is needed - we use the system heap.
 */

#include "arch/arch.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Memory Management Interface Implementation
// ============================================================================

/**
 * @brief Initialize architecture memory system
 *
 * For HOST, this is a no-op since we use standard malloc.
 */
void arch_memory_init(void) {
    // Nothing to initialize for standard malloc/free
}

/**
 * @brief Allocate memory
 *
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* arch_memory_alloc(size_t size) {
    return malloc(size);
}

/**
 * @brief Free allocated memory
 *
 * @param ptr Pointer to memory to free
 */
void arch_memory_free(void* ptr) {
    free(ptr);
}

/**
 * @brief Reallocate memory
 *
 * @param ptr Pointer to existing memory
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* arch_memory_realloc(void* ptr, size_t new_size) {
    return realloc(ptr, new_size);
}

/**
 * @brief Get amount of free memory available
 *
 * @return Free memory in bytes (returns 0 for HOST - not meaningful)
 */
size_t arch_memory_get_free(void) {
    // Not meaningful for HOST - system manages memory
    return 0;
}
