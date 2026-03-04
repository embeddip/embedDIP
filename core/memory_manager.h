/* ========================================================================== */
/*  File: memory_manager.h                                                    */
/*  Brief: Memory allocation interface for the EmbedDIP library               */
/*  SPDX-License-Identifier: BSD-3-Clause                                     */
/* ========================================================================== */
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#pragma once

/**
 * @file memory_manager.h
 * @brief Memory management interface for the EmbedDIP library.
 *
 * Provides abstraction over dynamic memory allocation, allowing
 * library internals to be decoupled from the platform’s native
 * allocator.
 *
 * Typical use cases:
 * - Centralized allocation tracking for debugging
 * - Custom allocators for embedded targets
 * - Memory pool management
 *
 * @note All allocations made via these functions should be freed
 *       using ::memory_free, not `free()`.
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup embedDIP_mem Memory Manager
 * @ingroup embedDIP_c_api
 * @brief Abstraction layer for dynamic memory allocation in EmbedDIP.
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize the memory manager.
     *
     * Should be called once at startup before any other memory functions.
     * If using a pool allocator, this function sets up the pool.
     */
    void memory_init(void);

    /**
     * @brief Allocate a block of memory.
     *
     * @param size Size of the block in bytes.
     * @return Pointer to the allocated block, or `NULL` if allocation fails.
     *
     * @note The returned memory is uninitialized.
     */
    void *memory_alloc(size_t size);

    /**
     * @brief Free a previously allocated block.
     *
     * @param ptr Pointer to the block to free. Safe to pass `NULL`.
     */
    void memory_free(void *ptr);

    /**
     * @brief Resize a previously allocated block.
     *
     * @param ptr      Pointer to the block to resize. May be `NULL` (equivalent to ::memory_alloc).
     * @param new_size New block size in bytes.
     * @return Pointer to the resized block, or `NULL` if allocation fails.
     *
     * @note The contents are preserved up to the lesser of the old and new sizes.
     */
    void *memory_realloc(void *ptr, size_t new_size);

#ifdef __cplusplus
}
#endif

/** @} */ /* end of embedDIP_mem */

#endif /* MEMORY_MANAGER_H */
