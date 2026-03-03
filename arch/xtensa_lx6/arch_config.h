/**
 * @file arch_config.h
 * @brief Xtensa LX6 architecture configuration
 *
 * This file defines the capabilities and configuration for Xtensa LX6
 * processors used in ESP32 chips:
 * - ESP32 (dual-core Xtensa LX6)
 * - ESP32-SOLO-1 (single-core Xtensa LX6)
 * - ESP32-PICO series
 *
 * Key Features:
 * - Xtensa LX6 cores running up to 240 MHz
 * - Dual-core with FreeRTOS
 * - Single-precision FPU
 * - DSP instructions
 * - ESP-DSP library support
 * - PSRAM support (4-8MB external)
 */

#ifndef ARCH_XTENSA_LX6_CONFIG_H
#define ARCH_XTENSA_LX6_CONFIG_H

// Architecture identification
#define ARCH_NAME "Xtensa LX6 (ESP32)"
#define ARCH_XTENSA_LX6 1

// Architecture capabilities
#define ARCH_HAS_FPU 1              // Single-precision FPU
#define ARCH_HAS_DSP 1              // DSP instructions available
#define ARCH_HAS_CACHE 0            // No cache (uses write-through)
#define ARCH_HAS_DUAL_CORE 1        // Two cores available
#define ARCH_HAS_WIFI 1             // Wi-Fi available
#define ARCH_HAS_BLUETOOTH 1        // Bluetooth available

// FFT configuration
#define ARCH_MAX_FFT_SIZE 4096      // Maximum FFT size supported by ESP-DSP
#define ARCH_MIN_FFT_SIZE 16        // Minimum FFT size

// Memory configuration
#define ARCH_ALIGNMENT 4            // 4-byte alignment
#define ARCH_HAS_PSRAM 1            // PSRAM (external SRAM) available

// Performance characteristics
#define ARCH_MAX_CPU_FREQ_MHZ 240   // Maximum CPU frequency
#define ARCH_DEFAULT_CPU_FREQ_MHZ 160  // Default CPU frequency

// ESP-DSP library availability
#define ARCH_HAS_ESP_DSP 1

// Arduino framework compatibility
#define ARCH_ARDUINO_COMPATIBLE 1

// Architecture capability flags for runtime detection
#define ARCH_CAPABILITIES ( \
    ARCH_CAP_FPU | \
    ARCH_CAP_DSP | \
    ARCH_CAP_DUAL_CORE \
)

#endif // ARCH_XTENSA_LX6_CONFIG_H
