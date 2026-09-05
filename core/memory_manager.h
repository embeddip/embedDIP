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

#include "core/error.h"

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
 * @brief Named storage region for buffers shared with hardware accelerators.
 */
typedef enum {
    EMBEDDIP_MEMORY_REGION_DEFAULT = 0,
    EMBEDDIP_MEMORY_REGION_FAST_SRAM,
    EMBEDDIP_MEMORY_REGION_DMA,
    EMBEDDIP_MEMORY_REGION_PSRAM,
    EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH
} embeddip_memory_region_t;

/**
 * @brief Access and ownership properties of a buffer.
 */
typedef enum {
    EMBEDDIP_BUFFER_CPU_READ = 1u << 0,
    EMBEDDIP_BUFFER_CPU_WRITE = 1u << 1,
    EMBEDDIP_BUFFER_DMA_READ = 1u << 2,
    EMBEDDIP_BUFFER_DMA_WRITE = 1u << 3,
    EMBEDDIP_BUFFER_NPU_READ = 1u << 4,
    EMBEDDIP_BUFFER_NPU_WRITE = 1u << 5,
    EMBEDDIP_BUFFER_READ_ONLY = 1u << 6
} embeddip_buffer_flags_t;

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

/**
 * @brief Allocate writable storage from a named memory region.
 *
 * @param region    Requested memory region.
 * @param size      Number of bytes to allocate.
 * @param alignment Required power-of-two alignment in bytes.
 * @return Pointer to allocated storage, or `NULL` when unsupported or unavailable.
 */
void *memory_alloc_region(embeddip_memory_region_t region,
                          size_t size,
                          size_t alignment) EMBEDDIP_ALLOC_LIKE;

/**
 * @brief Make CPU writes visible to a cache-coherent device consumer.
 */
embeddip_status_t memory_cache_clean(const void *address, size_t size);

/**
 * @brief Make device writes visible to the CPU.
 */
embeddip_status_t memory_cache_invalidate(const void *address, size_t size);

#ifdef __cplusplus
}
#endif

/** @} */ /* end of embedDIP_mem */

#endif /* EMBEDDIP_MEMORY_MANAGER_H */
