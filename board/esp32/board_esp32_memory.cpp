#include <embedDIP_configs.h>

#ifdef TARGET_BOARD_ESP32

#include <core/memory_manager.h>
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h" // Required for ps_malloc
#include <Arduino.h>       // For Serial

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

void memory_init(void)
{
    // Fill if needed
    return;
}

void *memory_alloc(size_t size)
{
    void *ptr = ps_malloc(size);
    return ptr;
}

void memory_free(void *ptr)
{
    if (ptr)
    {
        free(ptr); // ps_malloc-allocated memory can be freed with free()
    }
    else
    {
        Serial.println("[memory_free] Attempted to free NULL pointer");
    }
}

void *memory_realloc(void *ptr, size_t new_size)
{
    if (!ptr)
    {
        return memory_alloc(new_size);
    }

    void *new_ptr = ps_malloc(new_size);
    if (new_ptr)
    {
        memcpy(new_ptr, ptr, new_size); // WARNING: if old size is unknown, this can overread
        free(ptr);
    }
    else
    {
        Serial.printf("[memory_realloc] Failed to reallocate %u bytes from %p\n", (unsigned int)new_size, ptr);
    }

    return new_ptr;
}

#endif
