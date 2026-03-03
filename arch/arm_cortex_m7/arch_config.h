/**
 * @file arch_config.h
 * @brief ARM Cortex-M7 architecture configuration
 *
 * This file defines the capabilities and configuration for ARM Cortex-M7
 * processors. This architecture is used in:
 * - STM32F7 series (STM32F746, STM32F767, etc.)
 * - STM32H7 series (STM32H743, STM32H753, etc.)
 * - NXP i.MX RT1060 series
 *
 * Key Features:
 * - ARM Cortex-M7 core running up to 400+ MHz
 * - Double-precision FPU (FPv5-D16)
 * - DSP instructions
 * - Instruction and data cache
 * - CMSIS-DSP library support
 */

#ifndef ARCH_ARM_CORTEX_M7_CONFIG_H
#define ARCH_ARM_CORTEX_M7_CONFIG_H

// Architecture identification
#define ARCH_NAME "ARM Cortex-M7"
#define ARCH_ARM_CORTEX_M7 1

// Architecture capabilities
#define ARCH_HAS_FPU 1          // Double-precision FPU available
#define ARCH_HAS_DSP 1          // DSP instructions available
#define ARCH_HAS_CACHE 1        // I-Cache and D-Cache available
#define ARCH_HAS_MPU 1          // Memory Protection Unit available

// FFT configuration
#define ARCH_MAX_FFT_SIZE 4096  // Maximum FFT size supported by CMSIS-DSP
#define ARCH_MIN_FFT_SIZE 16    // Minimum FFT size

// Memory configuration
#define ARCH_ALIGNMENT 4        // 4-byte alignment requirement
#define ARCH_CACHE_LINE_SIZE 32 // Cache line size in bytes

// Performance characteristics
#define ARCH_MAX_CPU_FREQ_MHZ 400  // Typical max frequency (varies by chip)

// CMSIS-DSP library availability
#define ARCH_HAS_CMSIS_DSP 1

// Architecture capability flags for runtime detection
#define ARCH_CAPABILITIES ( \
    ARCH_CAP_FPU | \
    ARCH_CAP_DSP | \
    ARCH_CAP_CACHE | \
    ARCH_CAP_SIMD \
)

#endif // ARCH_ARM_CORTEX_M7_CONFIG_H
