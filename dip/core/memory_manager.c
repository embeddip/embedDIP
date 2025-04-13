#include "memory_manager.h"

static size_t allocated_size = 0x1000;
void memory_manager_init(void)
{
    allocated_size = 0;
}

void *memory_alloc(size_t size)
{
    if (allocated_size + size > MEMORY_POOL_SIZE)
    {
        return NULL; // Out of memory
    }

    void *addr = MEMORY_POOL_BASE_ADDR + allocated_size;
    allocated_size += size; //TODO offset
    return addr;
}

void memory_reset(void)
{
    allocated_size = 0;
}

size_t memory_get_allocated_size(void)
{
    return allocated_size;
}
