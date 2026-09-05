// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "core/memory_manager.h"

#include <limits.h>
#include <stdalign.h>
#include <stdint.h>

#include <stm32n6xx.h>

#include <configs.h>

typedef struct {
    uintptr_t start;
    uintptr_t end;
    uintptr_t cursor;
} n6_memory_range_t;

static n6_memory_range_t fast_sram;
static n6_memory_range_t dma_memory;
static n6_memory_range_t psram;
static int memory_initialized;

static void n6_reset_range(n6_memory_range_t *range, uint8_t *start, uint8_t *end)
{
    range->start = (uintptr_t)start;
    range->end = (uintptr_t)end;
    range->cursor = range->start;
}

void memory_init(uintptr_t pool_start_addr)
{
    (void)pool_start_addr;
    n6_reset_range(&fast_sram, __embeddip_fast_sram_start__, __embeddip_fast_sram_end__);
    n6_reset_range(&dma_memory, __embeddip_dma_start__, __embeddip_dma_end__);
    n6_reset_range(&psram, __embeddip_psram_start__, __embeddip_psram_end__);
    memory_initialized = 1;
}

static int n6_alignment_is_power_of_two(size_t alignment)
{
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

static n6_memory_range_t *n6_range_for_region(embeddip_memory_region_t region)
{
    switch (region) {
    case EMBEDDIP_MEMORY_REGION_DEFAULT:
    case EMBEDDIP_MEMORY_REGION_FAST_SRAM:
        return &fast_sram;
    case EMBEDDIP_MEMORY_REGION_DMA:
        return &dma_memory;
    case EMBEDDIP_MEMORY_REGION_PSRAM:
        return &psram;
    case EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH:
    default:
        return NULL;
    }
}

static embeddip_status_t n6_allocate_region(embeddip_memory_region_t region,
                                            size_t size,
                                            size_t alignment,
                                            void **allocation)
{
    n6_memory_range_t *range;
    uintptr_t aligned_cursor;
    uintptr_t alignment_mask;

    if (allocation == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    *allocation = NULL;

    if (!n6_alignment_is_power_of_two(alignment)) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (size == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (region == EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    range = n6_range_for_region(region);
    if (range == NULL) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (!memory_initialized) {
        memory_init(0u);
        range = n6_range_for_region(region);
    }

    alignment_mask = (uintptr_t)alignment - 1u;
    if (range->end < range->start || range->cursor > UINTPTR_MAX - alignment_mask) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    aligned_cursor = (range->cursor + alignment_mask) & ~alignment_mask;
    if (aligned_cursor > range->end || size > range->end - aligned_cursor) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    range->cursor = aligned_cursor + size;
    *allocation = (void *)aligned_cursor;
    return EMBEDDIP_OK;
}

void *embeddip_board_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment)
{
    void *allocation;

    if (n6_allocate_region(region, size, alignment, &allocation) != EMBEDDIP_OK) {
        return NULL;
    }
    return allocation;
}

void *memory_alloc(size_t size)
{
    return embeddip_board_alloc_region(EMBEDDIP_MEMORY_REGION_DEFAULT, size, alignof(max_align_t));
}

void memory_free(void *ptr)
{
    (void)ptr;
}

void *memory_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL) {
        return memory_alloc(new_size);
    }
    return NULL;
}

static embeddip_status_t
n6_cache_span(const void *address, size_t size, uintptr_t *rounded_start, int32_t *rounded_size)
{
    const uintptr_t line_mask = (uintptr_t)EMBEDDIP_N6_CACHE_LINE_BYTES - 1u;
    uintptr_t start;
    uintptr_t end;
    uintptr_t rounded_end;
    uintptr_t span;

    if (address == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (size == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    start = (uintptr_t)address;
    if (size > UINTPTR_MAX - start) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    end = start + size;
    if (end > UINTPTR_MAX - line_mask) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }

    *rounded_start = start & ~line_mask;
    rounded_end = (end + line_mask) & ~line_mask;
    span = rounded_end - *rounded_start;
    if (span > INT32_MAX) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }

    *rounded_size = (int32_t)span;
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_board_cache_clean(const void *address, size_t size)
{
    uintptr_t rounded_start;
    int32_t rounded_size;
    embeddip_status_t status = n6_cache_span(address, size, &rounded_start, &rounded_size);

    if (status != EMBEDDIP_OK) {
        return status;
    }
    SCB_CleanDCache_by_Addr((void *)rounded_start, rounded_size);
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_board_cache_invalidate(const void *address, size_t size)
{
    uintptr_t rounded_start;
    int32_t rounded_size;
    embeddip_status_t status = n6_cache_span(address, size, &rounded_start, &rounded_size);

    if (status != EMBEDDIP_OK) {
        return status;
    }
    SCB_InvalidateDCache_by_Addr((void *)rounded_start, rounded_size);
    return EMBEDDIP_OK;
}
