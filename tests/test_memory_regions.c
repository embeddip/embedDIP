#include <assert.h>
#include <stdint.h>

#include <core/error.h>
#include <core/memory_manager.h>

#include "memory_test_hooks.h"

int main(void)
{
    void *fast = memory_alloc_region(EMBEDDIP_MEMORY_REGION_FAST_SRAM, 64u, 32u);
    void *default_region = memory_alloc_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 64u, 8u);
    void *dma = memory_alloc_region(EMBEDDIP_MEMORY_REGION_DMA, 64u, 8u);
    void *psram = memory_alloc_region(EMBEDDIP_MEMORY_REGION_PSRAM, 64u, 8u);

    assert(fast != NULL && ((uintptr_t)fast % 32u) == 0u);
    assert(default_region != NULL);
    assert(dma != NULL);
    assert(psram != NULL);
    assert(memory_alloc_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 0u, 8u) == NULL);
    assert(memory_alloc_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 64u, 3u) == NULL);
    assert(memory_alloc_region(EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH, 64u, 8u) == NULL);

    assert(memory_cache_clean((const void *)0x1003u, 61u) == EMBEDDIP_OK);
    assert(memory_test_last_cache_range_start() == (uintptr_t)0x1000u);
    assert(memory_test_last_cache_range_size() == 64u);
    assert(memory_cache_invalidate((const void *)0x1020u, 32u) == EMBEDDIP_OK);
    assert(memory_test_last_cache_range_start() == (uintptr_t)0x1020u);
    assert(memory_test_last_cache_range_size() == 32u);
    assert(memory_cache_clean(NULL, 1u) == EMBEDDIP_ERROR_NULL_PTR);
    assert(memory_cache_invalidate((const void *)0x1000u, 0u) == EMBEDDIP_ERROR_INVALID_SIZE);
    assert(memory_cache_clean((const void *)(uintptr_t)(UINTPTR_MAX - 1u), 2u) ==
           EMBEDDIP_ERROR_OVERFLOW);

    memory_free(fast);
    memory_free(default_region);
    memory_free(dma);
    memory_free(psram);
    return 0;
}
