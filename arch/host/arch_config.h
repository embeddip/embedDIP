/**
 * @file arch_config.h
 * @brief HOST architecture configuration (x86/x64 for PC testing)
 *
 * Architecture: x86_64 or x86 (native host CPU)
 * Purpose: Software implementations for testing on PC
 *
 * This architecture provides software implementations of all
 * hardware-accelerated functions for testing purposes.
 */

#ifndef ARCH_HOST_CONFIG_H
#define ARCH_HOST_CONFIG_H

// ============================================================================
// Architecture Identification
// ============================================================================
#define ARCH_HOST 1
#define ARCH_NAME "HOST"
#define ARCH_DESCRIPTION "x86/x64 native (software implementations)"

// ============================================================================
// Architecture Capabilities
// ============================================================================

// CPU Features
#define ARCH_HAS_FPU 1              // Standard floating point
#define ARCH_FPU_DOUBLE 1           // Double precision support
#define ARCH_HAS_DSP 0              // No dedicated DSP instructions
#define ARCH_HAS_CACHE 1            // CPU has cache

// Memory Features
#define ARCH_HAS_MPU 0              // No memory protection unit
#define ARCH_HAS_EXTERNAL_MEMORY 0  // Uses standard system RAM
#define ARCH_MEMORY_ALIGNMENT 8     // 8-byte alignment for x64

// FFT Capabilities (software implementation)
#define ARCH_FFT_MAX_SIZE 8192      // Arbitrary limit for software FFT
#define ARCH_SUPPORTS_FFT 1
#define ARCH_SUPPORTS_IFFT 1
#define ARCH_FFT_2D 1               // 2D FFT support

// Performance Characteristics
#define ARCH_CPU_FREQUENCY_HZ 0     // Variable (depends on host CPU)
#define ARCH_IS_EMBEDDED 0          // This is a PC, not embedded

// ============================================================================
// Architecture-Specific Types
// ============================================================================

// Use standard types
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// Architecture Validation
// ============================================================================
#if !defined(__x86_64__) && !defined(__i386__) && !defined(__amd64__) && !defined(_M_X64) && !defined(_M_IX86)
    #warning "HOST architecture expects x86/x64, but may work on other platforms"
#endif

#endif // ARCH_HOST_CONFIG_H
