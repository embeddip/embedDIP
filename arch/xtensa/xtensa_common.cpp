// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef EMBED_DIP_ARCH_XTENSA

    #ifndef ESP32_COMMON_H
        #define ESP32_COMMON_H
        #include "board/common.h"
        #include "core/image.h"

        #include <float.h>
        #include <limits.h>
        #include <math.h>
        #include <stdint.h>
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>

        #include "Arduino.h"
        #include "dsps_fft2r.h"
        #include "esp_dsp.h"
        #include "esp_heap_caps.h"
        #include "esp_log.h"
        #include <board/common.h>
        #include <core/memory_manager.h>
        #include <esp32/rom/rtc.h>

        /* ========================================================================== */
        /*  ESP32 timing helpers: tic()/toc() returning elapsed CPU cycles            */
        /*  Supports Xtensa (ESP32/ESP32-S2/S3) and RISC-V (ESP32-C3/C6/H2)           */
        /* ========================================================================== */
        #include <stdint.h>

        /* Optionally let the build system define CPU freq in MHz (e.g., 240 for ESP32). */
        #ifndef EMBED_DIP_CPU_MHZ
            /* Try to pick up IDF config if present, else default to 240 MHz (classic ESP32). */
            #ifdef CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ
                #define EMBED_DIP_CPU_MHZ (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
            #else
                #define EMBED_DIP_CPU_MHZ (240U)
            #endif
        #endif

/* ---- Low-level cycle counter read, per-architecture ---------------------- */
static inline uint32_t edp_read_cycle_count(void)
{
        #if defined(__XTENSA__)
    /* Xtensa: read CCOUNT register */
    uint32_t ccount;
    __asm__ __volatile__("rsr.ccount %0" : "=a"(ccount));
    return ccount;

        #elif defined(__riscv)
    /* RISC-V: read lower 32 bits of cycle CSR */
    unsigned long long cycles;
    __asm__ __volatile__("rdcycle %0" : "=r"(cycles));
    return (uint32_t)cycles;

        #else
            /* Fallback: use a microsecond timer and convert to cycles.
               On Arduino-ESP32, micros() exists; on ESP-IDF, use esp_timer_get_time().
               Adjust below depending on your environment. */
            #if defined(ARDUINO) && defined(ARDUINO_ARCH_ESP32)
    extern "C" unsigned long micros(void);
    return (uint32_t)(micros() * (uint32_t)EMBED_DIP_CPU_MHZ);
            #elif defined(ESP_PLATFORM)
                #include "esp_timer.h"
    return (uint32_t)(esp_timer_get_time() * (uint32_t)EMBED_DIP_CPU_MHZ);
            #else
                #warning "No cycle counter available; returning 0."
    return 0U;
            #endif
        #endif
}

/* ---- Public API ---------------------------------------------------------- */
/**
 * @brief Start timing (records current CPU cycle count).
 */
void tic(void)
{
    /* Using a static makes API header-only friendly; you can move this to a .c file if preferred.
     */
    extern uint32_t __edp_tic_cycles;
    __edp_tic_cycles = edp_read_cycle_count();
}

/**
 * @brief Stop timing and return elapsed CPU cycles since last tic().
 * @return Elapsed CPU cycles (wrap-around safe).
 */
uint32_t toc(void)
{
    extern uint32_t __edp_tic_cycles;
    uint32_t now = edp_read_cycle_count();
    return (uint32_t)(now - __edp_tic_cycles);
}

/* Storage for tic()/toc() */
uint32_t __edp_tic_cycles = 0U;

    #endif
#endif
