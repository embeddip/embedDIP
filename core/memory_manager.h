// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_MEMORY_MANAGER_H
#define EMBEDDIP_MEMORY_MANAGER_H

/**
 * @file memory_manager.h
 * @brief Memory management interface for the EmbedDIP library.
 *
 * Provides abstraction over dynamic memory allocation.
 *
 */

#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup embedDIP_mem Memory Manager
 * @ingroup embedDIP_c_api
 * @brief Abstraction layer for dynamic memory allocation in EmbedDIP.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Compiler attributes for better diagnostics / optimization                  */
/* -------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
    #define EMBEDDIP_ALLOC_LIKE __attribute__((malloc, warn_unused_result))
    #define EMBEDDIP_WARN_UNUSED __attribute__((warn_unused_result))
#else
    #define EMBEDDIP_ALLOC_LIKE
    #define EMBEDDIP_WARN_UNUSED
#endif

/**
 * @brief Initialize the memory manager with default backend settings.
 *
 * Should be called once at startup before any other memory functions.
 * For pool allocators, this uses the board-specific default pool address.
 *
 * @param pool_start_addr Start address of the memory pool.
 */
void memory_init(uintptr_t pool_start_addr);

/**
 * @brief Allocate a block of memory.
 *
 * @param size Size of the block in bytes.
 * @return Pointer to the allocated block, or `NULL` if allocation fails.
 *
 */
void *memory_alloc(size_t size) EMBEDDIP_ALLOC_LIKE;

/**
 * @brief Free a previously allocated block.
 *
 * @param ptr Pointer to the block to free. Safe to pass `NULL`.
 */
void memory_free(void *ptr);

/**
 * @brief Resize a previously allocated block.
 *
 * @param ptr      Pointer to the block to resize. May be `NULL`
 *
 * @param new_size New block size in bytes.
 * @return Pointer to the resized block, or `NULL` if allocation fails.
 *
 */
void *memory_realloc(void *ptr, size_t new_size) EMBEDDIP_WARN_UNUSED;

#ifdef __cplusplus
}
#endif

/** @} */ /* end of embedDIP_mem */

#endif /* EMBEDDIP_MEMORY_MANAGER_H */
