/**
 * @file arch_memory.cpp
 * @brief Xtensa LX6 memory management for ESP32
 *
 * This implementation uses ESP32's heap allocator with PSRAM (external SPI RAM)
 * support. ESP32 has sophisticated memory management with:
 * - Internal SRAM (fast, limited)
 * - PSRAM (slower, abundant - 4-8MB)
 * - DMA-capable memory regions
 *
 * We use heap_caps_malloc() to allocate from PSRAM for large image buffers.
 *
 * @note ESP32's heap allocator is thread-safe (FreeRTOS)
 * @note PSRAM access is slower than internal SRAM but provides more space
 */

#include "arch/arch.h"
#include "arch_config.h"

#ifdef ARCH_XTENSA_LX6

#include "esp_heap_caps.h"
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Memory Allocation Strategy
// ============================================================================

// Use PSRAM for large allocations (better for image processing)
#define USE_PSRAM_FOR_ALLOC 1

#if USE_PSRAM_FOR_ALLOC
    #define MEMORY_ALLOC_CAPS MALLOC_CAP_SPIRAM  // PSRAM (external SPI RAM)
#else
    #define MEMORY_ALLOC_CAPS MALLOC_CAP_8BIT    // Internal SRAM
#endif

// ============================================================================
// Public API Implementation (C linkage for compatibility)
// ============================================================================

extern "C" {

void arch_memory_init(void) {
    // ESP32's heap allocator is already initialized by Arduino framework
    // Just print memory info for debugging

    Serial.println("[ARCH] Memory initialized");
    Serial.printf("[ARCH] Internal SRAM free: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    if (psramFound()) {
        Serial.printf("[ARCH] PSRAM detected: %d bytes free\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        Serial.printf("[ARCH] Using PSRAM for image allocations\n");
    } else {
        Serial.println("[ARCH] WARNING: No PSRAM detected! Using internal SRAM (limited)");
    }
}

void* arch_memory_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Try to allocate from PSRAM first (if available)
    void* ptr = heap_caps_malloc(size, MEMORY_ALLOC_CAPS);

    if (!ptr && psramFound()) {
        // PSRAM allocation failed, try internal SRAM as fallback
        Serial.printf("[ARCH] WARNING: PSRAM allocation failed (%d bytes), trying internal SRAM\n", size);
        ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT);
    }

    if (!ptr) {
        // Both failed
        Serial.printf("[ARCH] ERROR: Memory allocation failed (%d bytes)\n", size);
        Serial.printf("[ARCH] Free PSRAM: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        Serial.printf("[ARCH] Free Internal: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }

    return ptr;
}

void arch_memory_free(void* ptr) {
    if (!ptr) {
        return;  // NULL pointer is safe to free
    }

    // ESP32's heap allocator automatically determines which heap to free from
    free(ptr);
}

void* arch_memory_realloc(void* ptr, size_t new_size) {
    if (!ptr) {
        // NULL pointer - behave like malloc
        return arch_memory_alloc(new_size);
    }

    if (new_size == 0) {
        // Zero size - behave like free
        arch_memory_free(ptr);
        return NULL;
    }

    // Try to reallocate in PSRAM
    void* new_ptr = heap_caps_realloc(ptr, new_size, MEMORY_ALLOC_CAPS);

    if (!new_ptr && psramFound()) {
        // Realloc failed in PSRAM, try manual copy to internal SRAM
        Serial.printf("[ARCH] WARNING: Realloc failed in PSRAM, trying manual copy\n");

        new_ptr = heap_caps_malloc(new_size, MALLOC_CAP_8BIT);
        if (new_ptr) {
            // Get old size (approximation - copy more than needed is safe)
            size_t old_size = heap_caps_get_allocated_size(ptr);
            size_t copy_size = (old_size < new_size) ? old_size : new_size;

            memcpy(new_ptr, ptr, copy_size);
            free(ptr);
        }
    }

    if (!new_ptr) {
        Serial.printf("[ARCH] ERROR: Memory reallocation failed (%d bytes)\n", new_size);
    }

    return new_ptr;
}

} // extern "C"

#endif // ARCH_XTENSA_LX6
