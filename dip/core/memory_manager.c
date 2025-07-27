#include "memory_manager.h"
#include "main.h"
#include <stdint.h>
#include <string.h>
#include <configs.h>

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

extern SDRAM_HandleTypeDef hsdram1;

void memory_init()
{

    static FMC_SDRAM_TimingTypeDef Timing;
    static FMC_SDRAM_CommandTypeDef Command;
    __IO uint32_t tmpmrd = 0;
    Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(&hsdram1, &Command, (uint32_t)0xFFFF);

    HAL_Delay(1);
    Command.CommandMode = FMC_SDRAM_CMD_PALL;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(&hsdram1, &Command, (uint32_t)0xFFFF);

    Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 8;
    Command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(&hsdram1, &Command, (uint32_t)0xFFFF);
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL | SDRAM_MODEREG_CAS_LATENCY_2 | SDRAM_MODEREG_OPERATING_MODE_STANDARD | SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = tmpmrd;
    HAL_SDRAM_SendCommand(&hsdram1, &Command, (uint32_t)0xFFFF);

    hsdram1.Instance->SDRTR |= ((uint32_t)((1292) << 1));

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
