// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#if !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#include <stdalign.h>
#include <stdlib.h>

#include <core/memory_manager.h>

#define EMBEDDIP_HOST_CACHE_LINE_BYTES 32u

static uintptr_t last_cache_range_start;
static size_t last_cache_range_size;

static void memory_test_record_cache_range(const void *address, size_t size)
{
    last_cache_range_start = (uintptr_t)address;
    last_cache_range_size = size;
}

void memory_init(uintptr_t ignored)
{
    (void)ignored;
}

void *memory_alloc(size_t size)
{
    return size == 0u ? NULL : malloc(size);
}

void memory_free(void *ptr)
{
    free(ptr);
}

void *memory_realloc(void *ptr, size_t size)
{
    if (size == 0u) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, size);
}

void *embeddip_board_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment)
{
    void *memory = NULL;

    if (size == 0u || region == EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH) {
        return NULL;
    }
    if (region != EMBEDDIP_MEMORY_REGION_DEFAULT && region != EMBEDDIP_MEMORY_REGION_FAST_SRAM &&
        region != EMBEDDIP_MEMORY_REGION_DMA && region != EMBEDDIP_MEMORY_REGION_PSRAM) {
        return NULL;
    }
    if (alignment <= alignof(max_align_t)) {
        return malloc(size);
    }
    if (posix_memalign(&memory, alignment, size) != 0) {
        return NULL;
    }

    return memory;
}

embeddip_status_t embeddip_board_cache_clean(const void *address, size_t size)
{
    memory_test_record_cache_range(address, size);
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_board_cache_invalidate(const void *address, size_t size)
{
    memory_test_record_cache_range(address, size);
    return EMBEDDIP_OK;
}

uintptr_t memory_test_last_cache_range_start(void)
{
    return last_cache_range_start;
}

size_t memory_test_last_cache_range_size(void)
{
    return last_cache_range_size;
}
