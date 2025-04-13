#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

// TODO taşınacak

#define SDRAM_BANK_ADDR ((uint32_t)0xC0000000)

#define BYTES_U8 1
#define BYTES_U16 2
#define BYTES_U24 3
#define BYTES_F32 4
#define BYTES_PER_PIXEL 4 // if you're forcing float allocation

// Configurable base address and size of the SRAM memory pool
#define MEMORY_POOL_BASE_ADDR ((uint8_t *)SDRAM_BANK_ADDR)
#define MEMORY_POOL_SIZE (0x10000000) // 64MB SDRAM

// Initializes memory pool (optional if zero-initialized)
void memory_manager_init(void);

// Allocates memory from the pool
void *memory_alloc(size_t size);

// Resets the memory pool (e.g., at frame start or soft reset)
void memory_reset(void);

// Returns current allocated size (for diagnostics or debug)
size_t memory_get_allocated_size(void);

#endif // MEMORY_MANAGER_H
