// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef EMBED_DIP_BOARD_ESP32

    #include <stdlib.h>

    #include "esp_heap_caps.h"  // Required for ps_malloc
    #include <Arduino.h>        // For Serial
    #include <core/memory_manager.h>

    #define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

void memory_init(uintptr_t pool_start_addr)
{
    (void)pool_start_addr;

    // Check if PSRAM is available
    if (ESP.getPsramSize() > 0) {
        Serial.printf("[MEMORY] PSRAM available: %u bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("[MEMORY] WARNING: No PSRAM detected, using heap");
    }
}

void *memory_alloc(size_t size)
{
    void *ptr = nullptr;

    // Try PSRAM first if available
    if (ESP.getPsramSize() > 0) {
        ptr = ps_malloc(size);
    }

    // Fallback to regular heap if PSRAM not available or allocation failed
    if (!ptr) {
        ptr = malloc(size);
        if (!ptr) {
            Serial.printf("[MEMORY] ERROR: Failed to allocate %u bytes\n", (unsigned int)size);
        }
    }

    return ptr;
}

void memory_free(void *ptr)
{
    if (ptr) {
        free(ptr);  // ps_malloc-allocated memory can be freed with free()
    } else {
        Serial.println("[memory_free] Attempted to free NULL pointer");
    }
}

void *memory_realloc(void *ptr, size_t new_size)
{
    if (!ptr) {
        return memory_alloc(new_size);
    }

    if (new_size == 0) {
        memory_free(ptr);
        return NULL;
    }

    // Use realloc so the runtime copies only the old allocation size.
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        Serial.printf("[memory_realloc] Failed to reallocate %u bytes from %p\n",
                      (unsigned int)new_size,
                      ptr);
    }

    return new_ptr;
}

#endif
