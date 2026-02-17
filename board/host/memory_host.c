/* ========================================================================== */
/*  File: memory_host.c                                                       */
/*  Brief: Host platform memory manager (uses standard malloc/free)           */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "core/memory_manager.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize the memory manager for host platform.
 *
 * On host builds, this is a no-op since we use standard malloc/free.
 */
void memory_init(void)
{
    /* No initialization needed for standard malloc/free */
}

/**
 * @brief Allocate memory using standard malloc.
 *
 * @param size Size in bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *memory_alloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    return malloc(size);
}

/**
 * @brief Free memory using standard free.
 *
 * @param ptr Pointer to memory to free. NULL is safe.
 */
void memory_free(void *ptr)
{
    free(ptr);
}

/**
 * @brief Reallocate memory using standard realloc.
 *
 * @param ptr Pointer to existing memory (may be NULL).
 * @param new_size New size in bytes.
 * @return Pointer to reallocated memory, or NULL on failure.
 */
void *memory_realloc(void *ptr, size_t new_size)
{
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, new_size);
}
