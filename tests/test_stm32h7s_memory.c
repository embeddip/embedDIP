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

#include "board/stm32h7s/board_stm32h7s_memory.c"

int main(void)
{
    alignas(32) uint8_t cache_span[96];
    void *allocation = (void *)(uintptr_t)1u;

    memory_init(0u);

    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 40u, 32u, &allocation) == EMBEDDIP_OK);
    assert(allocation == fast_storage);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_FAST_SRAM, 64u, 1u, &allocation) ==
           EMBEDDIP_ERROR_OUT_OF_MEMORY);
    assert(allocation == NULL);

    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DMA, 16u, 3u, &allocation) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(allocation == NULL);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DMA, sizeof(dma_storage), 8u, &allocation) == EMBEDDIP_OK);
    assert(allocation == dma_storage);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_PSRAM, sizeof(psram_storage), 16u, &allocation) == EMBEDDIP_OK);
    assert(allocation == psram_storage);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH, 1u, 1u, &allocation) ==
           EMBEDDIP_ERROR_NOT_SUPPORTED);
    assert(allocation == NULL);

    assert(embeddip_board_cache_clean(cache_span + 3u, 33u) == EMBEDDIP_OK);
    assert(last_clean_address == cache_span);
    assert(last_clean_size == 64);
    assert(embeddip_board_cache_invalidate(cache_span + 31u, 2u) == EMBEDDIP_OK);
    assert(last_invalidate_address == cache_span);
    assert(last_invalidate_size == 64);

    /* Single-level LIFO reclaim: freeing the most recent allocation rewinds the
     * bump cursor so the space (and alignment padding) is reused. */
    memory_init(0u);
    void *a = NULL, *b = NULL, *c = NULL;
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 40u, 8u, &a) == EMBEDDIP_OK);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 40u, 8u, &b) == EMBEDDIP_OK);
    assert(a == fast_storage);
    assert(b != a);
    /* Free the top (b): a subsequent same-size alloc must reuse b's address. */
    memory_free(b);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 40u, 8u, &c) == EMBEDDIP_OK);
    assert(c == b);
    /* Freeing a non-top block (a, while c is live) is a safe no-op: c stays put
     * and the next alloc does not clobber it. */
    memory_free(a);
    void *d = NULL;
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 8u, 8u, &d) == EMBEDDIP_OK);
    assert(d != c);
    /* Repeated alloc/free of the top must not advance the cursor (no leak) --
     * this is the fft.c per-call temp pattern that was exhausting the pool. */
    for (int k = 0; k < 1000; ++k) {
        void *t = NULL;
        assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 8u, 8u, &t) == EMBEDDIP_OK);
        memory_free(t);
    }

    return 0;
}
