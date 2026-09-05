// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "core/memory_manager.h"

#include <limits.h>

#define EMBEDDIP_CACHE_LINE_BYTES 32u

void *embeddip_board_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment);
embeddip_status_t embeddip_board_cache_clean(const void *address, size_t size);
embeddip_status_t embeddip_board_cache_invalidate(const void *address, size_t size);

static int memory_region_is_writable(embeddip_memory_region_t region)
{
    return region == EMBEDDIP_MEMORY_REGION_DEFAULT || region == EMBEDDIP_MEMORY_REGION_FAST_SRAM ||
           region == EMBEDDIP_MEMORY_REGION_DMA || region == EMBEDDIP_MEMORY_REGION_PSRAM;
}

static int memory_alignment_is_power_of_two(size_t alignment)
{
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

static embeddip_status_t memory_cache_apply(const void *address,
                                            size_t size,
                                            embeddip_status_t (*operation)(const void *, size_t))
{
    const uintptr_t line_mask = (uintptr_t)EMBEDDIP_CACHE_LINE_BYTES - 1u;
    uintptr_t raw_address;
    uintptr_t rounded_address;
    uintptr_t end_address;
    uintptr_t rounded_end_address;

    if (address == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (size == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    raw_address = (uintptr_t)address;
    if (size > UINTPTR_MAX - raw_address) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    end_address = raw_address + size;
    if (end_address > UINTPTR_MAX - line_mask) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }

    rounded_address = raw_address & ~line_mask;
    rounded_end_address = (end_address + line_mask) & ~line_mask;

    return operation((const void *)rounded_address, rounded_end_address - rounded_address);
}

void *memory_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment)
{
    if (size == 0u || !memory_alignment_is_power_of_two(alignment) ||
        !memory_region_is_writable(region)) {
        return NULL;
    }

    return embeddip_board_alloc_region(region, size, alignment);
}

embeddip_status_t memory_cache_clean(const void *address, size_t size)
{
    return memory_cache_apply(address, size, embeddip_board_cache_clean);
}

embeddip_status_t memory_cache_invalidate(const void *address, size_t size)
{
    return memory_cache_apply(address, size, embeddip_board_cache_invalidate);
}
