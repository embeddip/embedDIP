#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "core/memory_manager.h"

alignas(32) uint8_t fast_storage[96];
alignas(32) uint8_t dma_storage[64];
alignas(32) uint8_t psram_storage[48];
alignas(32) uint8_t flash_storage[32];

__asm__(".globl __embeddip_fast_sram_start__\n"
        ".set __embeddip_fast_sram_start__, fast_storage\n"
        ".globl __embeddip_fast_sram_end__\n"
        ".set __embeddip_fast_sram_end__, fast_storage + 96\n"
        ".globl __embeddip_dma_start__\n"
        ".set __embeddip_dma_start__, dma_storage\n"
        ".globl __embeddip_dma_end__\n"
        ".set __embeddip_dma_end__, dma_storage + 64\n"
        ".globl __embeddip_psram_start__\n"
        ".set __embeddip_psram_start__, psram_storage\n"
        ".globl __embeddip_psram_end__\n"
        ".set __embeddip_psram_end__, psram_storage + 48\n"
        ".globl __embeddip_xspi_flash_start__\n"
        ".set __embeddip_xspi_flash_start__, flash_storage\n"
        ".globl __embeddip_xspi_flash_end__\n"
        ".set __embeddip_xspi_flash_end__, flash_storage + 32\n");

static void *last_clean_address;
static int32_t last_clean_size;
static void *last_invalidate_address;
static int32_t last_invalidate_size;

void SCB_CleanDCache_by_Addr(void *address, int32_t size)
{
    last_clean_address = address;
    last_clean_size = size;
}

void SCB_InvalidateDCache_by_Addr(void *address, int32_t size)
{
    last_invalidate_address = address;
    last_invalidate_size = size;
}

#include "board/stm32n6/board_stm32n6_memory.c"

int main(void)
{
    alignas(32) uint8_t cache_span[96];
    void *allocation = (void *)(uintptr_t)1u;

    memory_init(0u);

    assert(n6_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 40u, 32u, &allocation) == EMBEDDIP_OK);
    assert(allocation == fast_storage);
    assert(n6_allocate_region(EMBEDDIP_MEMORY_REGION_FAST_SRAM, 64u, 1u, &allocation) ==
           EMBEDDIP_ERROR_OUT_OF_MEMORY);
    assert(allocation == NULL);

    assert(n6_allocate_region(EMBEDDIP_MEMORY_REGION_DMA, 16u, 3u, &allocation) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(allocation == NULL);
    assert(n6_allocate_region(EMBEDDIP_MEMORY_REGION_DMA, sizeof(dma_storage), 8u, &allocation) == EMBEDDIP_OK);
    assert(allocation == dma_storage);
    assert(n6_allocate_region(EMBEDDIP_MEMORY_REGION_PSRAM, sizeof(psram_storage), 16u, &allocation) == EMBEDDIP_OK);
    assert(allocation == psram_storage);
    assert(n6_allocate_region(EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH, 1u, 1u, &allocation) ==
           EMBEDDIP_ERROR_NOT_SUPPORTED);
    assert(allocation == NULL);

    assert(embeddip_board_cache_clean(cache_span + 3u, 33u) == EMBEDDIP_OK);
    assert(last_clean_address == cache_span);
    assert(last_clean_size == 64);
    assert(embeddip_board_cache_invalidate(cache_span + 31u, 2u) == EMBEDDIP_OK);
    assert(last_invalidate_address == cache_span);
    assert(last_invalidate_size == 64);

    return 0;
}
