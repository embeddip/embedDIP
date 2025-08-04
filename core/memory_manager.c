#include <embedDIP_configs.h>

#ifdef TARGET_BOARD_STM32F7

#include "memory_manager.h"
#include <stdint.h>
#include <string.h>

#define MEMORY_POOL_SIZE (1024 * 1024 * 8) // 8MB
static uint8_t *memory_pool = ((uint8_t *)SDRAM_BANK_ADDR + 0x1000);

typedef struct MemoryBlock
{
    size_t size;
    struct MemoryBlock *next;
    int is_free;
} MemoryBlock;

#define ALIGN4(s) (((s) + 3) & ~3)
#define BLOCK_SIZE sizeof(MemoryBlock)

static MemoryBlock *free_list = NULL;
static int initialized = 0;

void memory_init()
{
    if (initialized)
        return;

    free_list = (MemoryBlock *)memory_pool;
    free_list->size = MEMORY_POOL_SIZE - BLOCK_SIZE;
    free_list->next = NULL;
    free_list->is_free = 1;

    initialized = 1;
}

void *memory_alloc(size_t size)
{
    if (!initialized)
        memory_init();

    size = ALIGN4(size);

    MemoryBlock *curr = free_list;

    while (curr)
    {
        if (curr->is_free && curr->size >= size)
        {
            uintptr_t curr_addr = (uintptr_t)curr;
            uintptr_t pool_end = (uintptr_t)memory_pool + MEMORY_POOL_SIZE;
            uintptr_t next_block_addr = curr_addr + BLOCK_SIZE + size;

            if (curr->size >= size + BLOCK_SIZE + 4 && next_block_addr + BLOCK_SIZE < pool_end)
            {
                MemoryBlock *new_block = (MemoryBlock *)(next_block_addr);
                new_block->size = curr->size - size - BLOCK_SIZE;
                new_block->next = curr->next;
                new_block->is_free = 1;

                curr->next = new_block;
                curr->size = size;
            }

            curr->is_free = 0;
            return (void *)((uint8_t *)curr + BLOCK_SIZE);
        }

        curr = curr->next;
    }

    return NULL;
}

void memory_free(void *ptr)
{
    if (!ptr)
        return;

    uintptr_t pool_start = (uintptr_t)memory_pool;
    uintptr_t pool_end = pool_start + MEMORY_POOL_SIZE;
    uintptr_t addr = (uintptr_t)ptr;

    if (addr < pool_start || addr >= pool_end)
        return;

    if (!initialized)
        memory_init();

    MemoryBlock *block = (MemoryBlock *)((uint8_t *)ptr - BLOCK_SIZE);
    block->is_free = 1;

    // Merge adjacent free blocks
    MemoryBlock *curr = free_list;
    while (curr && curr->next)
    {
        if (curr->is_free && curr->next->is_free)
        {
            curr->size += BLOCK_SIZE + curr->next->size;
            curr->next = curr->next->next;
        }
        else
        {
            curr = curr->next;
        }
    }
}

void *memory_realloc(void *ptr, size_t new_size)
{
    if (!ptr)
        return memory_alloc(new_size);

    if (!initialized)
        memory_init();

    MemoryBlock *block = (MemoryBlock *)((uint8_t *)ptr - BLOCK_SIZE);

    if (block->size >= new_size)
        return ptr;

    void *new_ptr = memory_alloc(new_size);
    if (new_ptr)
    {
        memcpy(new_ptr, ptr, block->size);
        memory_free(ptr);
    }
    return new_ptr;
}

#endif // TARGET_BOARD_STM32F7
