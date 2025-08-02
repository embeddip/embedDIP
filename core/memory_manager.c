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

static MemoryBlock *free_list;

void memory_init()
{
    free_list = (MemoryBlock *)memory_pool;
    free_list->size = MEMORY_POOL_SIZE - BLOCK_SIZE;
    free_list->next = NULL;
    free_list->is_free = 1;
}

void *memory_alloc(size_t size)
{
    size = ALIGN4(size);

    if (free_list == NULL)
    {
        memory_init();
    }

    MemoryBlock *curr = free_list;

    while (curr)
    {
        if (curr->is_free && curr->size >= size)
        {
            uintptr_t curr_addr = (uintptr_t)curr;
            uintptr_t pool_end = (uintptr_t)memory_pool + MEMORY_POOL_SIZE;
            uintptr_t next_block_addr = curr_addr + BLOCK_SIZE + size;

            // Case 1: block can be split
            if (curr->size >= size + BLOCK_SIZE + 4 && next_block_addr + BLOCK_SIZE < pool_end)
            {
                MemoryBlock *new_block = (MemoryBlock *)(next_block_addr);
                new_block->size = curr->size - size - BLOCK_SIZE;
                new_block->next = curr->next;
                new_block->is_free = 1;

                curr->next = new_block;
                curr->size = size;
            }
            // Case 2: allocate entire block
            curr->is_free = 0;
            return (void *)((uint8_t *)curr + BLOCK_SIZE);
        }

        curr = curr->next;
    }

    return NULL; // No suitable block
}

void memory_free(void *ptr)
{
    if (!ptr)
        return;

    MemoryBlock *block = (MemoryBlock *)((uint8_t *)ptr - BLOCK_SIZE);
    block->is_free = 1;

    if (free_list == NULL)
    {
        memory_init();
    }

    MemoryBlock *curr = free_list;

    // Merge adjacent free blocks
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

    if (free_list == NULL)
    {
        memory_init();
    }

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
