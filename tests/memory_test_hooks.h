#ifndef EMBEDDIP_MEMORY_TEST_HOOKS_H
#define EMBEDDIP_MEMORY_TEST_HOOKS_H

#include <stddef.h>
#include <stdint.h>

#if !defined(EMBED_DIP_BOARD_HOST)
    #error "memory test hooks are available only for the host board"
#endif

uintptr_t memory_test_last_cache_range_start(void);
size_t memory_test_last_cache_range_size(void);

#endif /* EMBEDDIP_MEMORY_TEST_HOOKS_H */
