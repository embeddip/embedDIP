#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

// TODO taşınacak

#define SDRAM_BANK_ADDR ((uint32_t)0xC0100000)

#define BYTES_U8 1
#define BYTES_U16 2
#define BYTES_U24 3
#define BYTES_F32 4
#define BYTES_PER_PIXEL 4 // if you're forcing float allocation

// Configurable base address and size of the SRAM memory pool

void memory_init(void);
void *memory_alloc(size_t size);
void memory_free(void *ptr);
void *memory_realloc(void *ptr, size_t new_size);

#endif // MEMORY_MANAGER_H
