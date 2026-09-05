// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "core/memory_manager.h"

#include <limits.h>
#include <stdalign.h>
#include <stdint.h>

#include <stm32h7s7xx.h>

#include <configs.h>

typedef struct {
    uintptr_t start;
    uintptr_t end;
    uintptr_t cursor;
    uintptr_t last_payload;     /* start of the most recent allocation, 0 if none */
    uintptr_t last_prev_cursor; /* cursor value to restore when that block is freed */
} h7s_memory_range_t;

static h7s_memory_range_t fast_sram;
static h7s_memory_range_t dma_memory;
static h7s_memory_range_t psram;
static int memory_initialized;

static void h7s_reset_range(h7s_memory_range_t *range, uint8_t *start, uint8_t *end)
{
    range->start = (uintptr_t)start;
    range->end = (uintptr_t)end;
    range->cursor = range->start;
    range->last_payload = 0u;
    range->last_prev_cursor = range->start;
}

void memory_init(uintptr_t pool_start_addr)
{
    (void)pool_start_addr;
    h7s_reset_range(&fast_sram, __embeddip_fast_sram_start__, __embeddip_fast_sram_end__);
    h7s_reset_range(&dma_memory, __embeddip_dma_start__, __embeddip_dma_end__);
    h7s_reset_range(&psram, __embeddip_psram_start__, __embeddip_psram_end__);
    memory_initialized = 1;
}

static int h7s_alignment_is_power_of_two(size_t alignment)
{
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

static h7s_memory_range_t *h7s_range_for_region(embeddip_memory_region_t region)
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

static embeddip_status_t h7s_allocate_region(embeddip_memory_region_t region,
                                             size_t size,
                                             size_t alignment,
                                             void **allocation)
{
    h7s_memory_range_t *range;
    uintptr_t aligned_cursor;
    uintptr_t alignment_mask;

    if (allocation == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    *allocation = NULL;

    if (!h7s_alignment_is_power_of_two(alignment)) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (size == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (region == EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    range = h7s_range_for_region(region);
    if (range == NULL) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (!memory_initialized) {
        memory_init(0u);
        range = h7s_range_for_region(region);
    }

    alignment_mask = (uintptr_t)alignment - 1u;
    if (range->end < range->start || range->cursor > UINTPTR_MAX - alignment_mask) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    aligned_cursor = (range->cursor + alignment_mask) & ~alignment_mask;
    if (aligned_cursor > range->end || size > range->end - aligned_cursor) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    /* Remember this allocation so a later memory_free() of it can rewind the
     * bump cursor (single-level LIFO reclaim). last_prev_cursor restores the
     * pre-alignment cursor, so alignment padding is reclaimed too. */
    range->last_prev_cursor = range->cursor;
    range->last_payload = aligned_cursor;
    range->cursor = aligned_cursor + size;
    *allocation = (void *)aligned_cursor;
    return EMBEDDIP_OK;
}

void *embeddip_board_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment)
{
    void *allocation;

    if (h7s_allocate_region(region, size, alignment, &allocation) != EMBEDDIP_OK) {
        return NULL;
    }
    return allocation;
}

void *memory_alloc(size_t size)
{
    return embeddip_board_alloc_region(EMBEDDIP_MEMORY_REGION_DEFAULT, size, alignof(max_align_t));
}

/* Single-level LIFO reclaim: if ptr is the most recent allocation in its
 * region, rewind the bump cursor so the space is reused. Freeing anything
 * other than the current top is a safe no-op (the space is reclaimed once the
 * blocks above it are freed). This matches how embedDIP allocates and frees
 * transient scratch (e.g. imgproc/fft.c's per-call temp buffer, allocated and
 * freed with nothing live above it), so a long-running tracker does not leak.
 * ponytail: top-of-region reclaim only; a full free-list would be needed for
 * arbitrary out-of-order reclamation, which nothing here requires. */
void memory_free(void *ptr)
{
    h7s_memory_range_t *ranges[3];
    uintptr_t addr = (uintptr_t)ptr;
    int i;

    if (ptr == NULL) {
        return;
    }
    ranges[0] = &fast_sram;
    ranges[1] = &dma_memory;
    ranges[2] = &psram;
    for (i = 0; i < 3; ++i) {
        h7s_memory_range_t *range = ranges[i];
        if (addr >= range->start && addr < range->end) {
            if (addr == range->last_payload) {
                range->cursor = range->last_prev_cursor;
                range->last_payload = 0u;
            }
            return;
        }
    }
}

void *memory_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL) {
        return memory_alloc(new_size);
    }
    return NULL;
}

static embeddip_status_t
h7s_cache_span(const void *address, size_t size, uintptr_t *rounded_start, int32_t *rounded_size)
{
    const uintptr_t line_mask = (uintptr_t)EMBEDDIP_H7S_CACHE_LINE_BYTES - 1u;
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
    embeddip_status_t status = h7s_cache_span(address, size, &rounded_start, &rounded_size);

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
    embeddip_status_t status = h7s_cache_span(address, size, &rounded_start, &rounded_size);

    if (status != EMBEDDIP_OK) {
        return status;
    }
    SCB_InvalidateDCache_by_Addr((void *)rounded_start, rounded_size);
    return EMBEDDIP_OK;
}
