# embedDIP Architecture Refactoring Design Document

**Version:** 1.0
**Date:** March 2026
**Author:** embedDIP Development Team
**Status:** Implementation In Progress

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Problem Statement](#problem-statement)
3. [Current Architecture Analysis](#current-architecture-analysis)
4. [Proposed Solution](#proposed-solution)
5. [Three-Layer Architecture Model](#three-layer-architecture-model)
6. [Implementation Details](#implementation-details)
7. [Migration Guide](#migration-guide)
8. [Benefits and Trade-offs](#benefits-and-trade-offs)
9. [Testing Strategy](#testing-strategy)
10. [Glossary](#glossary)

---

## 1. Executive Summary

### 1.1 Overview

This document describes a fundamental architectural refactoring of the embedDIP library to separate **architecture-specific code** (CPU/instruction set) from **board-specific code** (hardware peripherals and pin mappings). The current codebase conflates these two distinct layers, making it difficult to support multiple boards with the same CPU architecture or reuse optimized algorithms across different hardware platforms.

### 1.2 Key Problems Identified

1. **Layer Confusion**: Architecture code (ARM Cortex-M7 FFT) is mixed with board code (pin mappings, memory addresses)
2. **Code Duplication**: Cannot reuse ARM Cortex-M7 optimizations across different STM32 boards
3. **Limited Scalability**: Adding a new board requires duplicating architecture-specific code
4. **Unclear Dependencies**: Build system cannot distinguish between CPU requirements and hardware requirements

### 1.3 Proposed Solution

Introduce a **three-layer architecture**:

```
Application Layer (User Code)
        ↓
Board Layer (Hardware: pins, peripherals, memory maps)
        ↓
Architecture Layer (CPU: FFT, DSP, memory APIs)
        ↓
Portable Layer (Pure C algorithms)
```

### 1.4 Expected Outcomes

- **Code Reusability**: Share ARM Cortex-M7 FFT across all Cortex-M7 boards
- **Clear Separation**: Architecture code independent of board specifics
- **Easy Expansion**: Add new boards by writing only board-specific configuration
- **Better Testing**: Test architecture features independently from hardware

---

## 2. Problem Statement

### 2.1 Current Issues

#### Issue 1: Conflated Defines

**Current Code:**
```c
#ifdef TARGET_BOARD_STM32F7
    #include "arm_math.h"  // Architecture-specific
    #define SDRAM_BANK_ADDR 0xC0000000  // Board-specific
#endif
```

**Problem**: `TARGET_BOARD_STM32F7` means both "Cortex-M7 CPU" AND "specific board with specific memory layout". This prevents:
- Using the same CPU code on different STM32F7 boards
- Supporting non-STM32 Cortex-M7 boards (e.g., NXP i.MX RT1060)

#### Issue 2: Architecture Code in Board Directories

**File Location:** `board/stm32f7/board_stm32f7_fft.c`

**Contents:**
```c
arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier + ..., 0, 1);
```

**Problem**: This FFT code works on ANY ARM Cortex-M7 CPU, not just STM32F746G-Discovery. It's architecture code placed in a board-specific location.

**Real-World Impact:**
- To support STM32F767ZI-Nucleo (also Cortex-M7), we must copy-paste this entire file
- If we find a bug in the FFT, we must fix it in multiple places
- Cannot easily add support for NXP i.MX RT1060 (Cortex-M7) without code duplication

#### Issue 3: Board-Specific Memory Addresses Hardcoded

**File:** `board/stm32f7/board_stm32f7_memory.c`
```c
#define SDRAM_BANK_ADDR ((uint32_t)0xC0000000)  // STM32F746G-Discovery specific
#define MEMORY_POOL_SIZE (1024 * 1024 * 8)      // 8MB SDRAM on this board
```

**Problem**: Different STM32F7 boards have different memory configurations:
- STM32F746G-Discovery: 8MB SDRAM at 0xC0000000
- STM32F767ZI-Nucleo: No external SDRAM (only internal RAM)
- STM32H743ZI-Nucleo: 16MB SDRAM at different address

Hardcoding board-specific addresses in architecture code prevents reusability.

### 2.2 Example Scenarios Where Current Design Fails

#### Scenario A: Adding STM32F767ZI-Nucleo Support

**Requirements:**
- Same CPU: ARM Cortex-M7 (can use same FFT code)
- Different board: Different pins, no SDRAM, different camera module
- Expected: Only write board-specific code

**Current Reality:**
1. Must copy entire `board/stm32f7/` directory
2. Must modify FFT code location (even though FFT is identical)
3. Must maintain two copies of architecture code
4. Bug fixes must be applied twice

**This violates DRY principle (Don't Repeat Yourself).**

#### Scenario B: Adding OV5640 Camera to ESP32 Board

**Requirements:**
- OV5640 sensor (same hardware sensor as STM32 boards)
- ESP32 CPU (different architecture)
- Expected: Reuse OV5640 driver, write ESP32 interface

**Current Reality:**
- OV5640 driver tightly coupled to STM32 HAL
- Cannot easily share sensor initialization code
- Must rewrite sensor configuration from scratch

### 2.3 Root Cause Analysis

The fundamental issue is **mixing concerns**:

1. **What CPU optimizations are available?** (Architecture concern)
2. **What hardware peripherals exist?** (Board concern)
3. **How are they connected?** (Board concern)

These are orthogonal concerns that should be separated.

---

## 3. Current Architecture Analysis

### 3.1 Code Layer Classification

We analyzed the entire codebase and classified files by their true nature:

#### 3.1.1 Portable Code (Platform-Independent)

**Location:** `core/`, `imgproc/`

**Characteristics:**
- Pure C code with no hardware dependencies
- Works on any platform (x86, ARM, Xtensa, RISC-V)
- Uses standard library only

**Examples:**
```c
// imgproc/filter.c
int filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx) {
    // Pure math - no hardware dependencies
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            // Convolution kernel computation
        }
    }
}
```

**Status:** ✅ Already properly structured

#### 3.1.2 Architecture-Specific Code (CPU/ISA-Dependent)

**Current Location:** `board/stm32f7/board_stm32f7_fft.c`, `board/esp32/board_esp32_fft.cpp`

**Characteristics:**
- Uses CPU-specific instructions or libraries
- Depends on ISA (Instruction Set Architecture)
- Can work on any board with that CPU

**ARM Cortex-M7 Example:**
```c
// Uses ARM CMSIS-DSP library (works on ANY Cortex-M7)
#include "arm_math.h"
arm_cfft_f32(&arm_cfft_sR_f32_len256, buffer, 0, 1);
```

**Xtensa LX6 Example:**
```cpp
// Uses ESP-DSP library (works on ESP32, ESP32-SOLO-1)
#include "esp_dsp.h"
dsps_fft2r_fc32(buffer, N);
dsps_bit_rev_fc32(buffer, N);
```

**Problem:** Currently mixed with board-specific code
**Solution:** Should be in `arch/` directory

#### 3.1.3 Board-Specific Code (Hardware-Dependent)

**Current Location:** Mixed throughout `board/`, `device/`

**Characteristics:**
- Pin mappings (which GPIO pin controls what)
- Memory layout (SDRAM addresses, sizes)
- Peripheral configurations
- Hardware initialization sequences

**Example:**
```c
// STM32F746G-Discovery specific
#define SDRAM_BANK_ADDR 0xC0000000  // Board memory map
#define CAMERA_PWR_PIN GPIO_PIN_13   // Board schematic
#define CAMERA_PWR_PORT GPIOH        // Board wiring
```

**Problem:** Mixed with architecture code
**Solution:** Should be in `boards/` directory with clear configuration

### 3.2 Dependency Analysis

We mapped dependencies between code layers:

```
┌─────────────────────────────────────────────────────┐
│ Application Code (main.c)                           │
│ - Uses: embedDIP API                                │
│ - Dependency: Board config (which hardware?)        │
└─────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────┐
│ Board Layer (STM32F746G-Discovery)                  │
│ - Pin Mappings: Camera on PH13, LCD on LTDC        │
│ - Memory Map: 8MB SDRAM at 0xC0000000              │
│ - Peripherals: DCMI, DMA2D, LTDC                   │
│ - Dependency: Architecture layer (CPU features)     │
└─────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────┐
│ Architecture Layer (ARM Cortex-M7)                  │
│ - FFT Implementation: arm_cfft_f32()                │
│ - Memory API: Generic allocation                    │
│ - Features: FPU, DSP instructions, cache            │
│ - Dependency: Portable layer                        │
└─────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────┐
│ Portable Layer (core/, imgproc/)                    │
│ - Image structures                                  │
│ - Generic algorithms                                │
│ - No dependencies                                   │
└─────────────────────────────────────────────────────┘
```

**Key Insight:** Dependencies should flow downward only. Currently, architecture code depends on board defines, which is backwards.

### 3.3 Current Build Flow

**CMakeLists.txt Logic:**
```cmake
if(EMBEDDIP_TARGET_PLATFORM STREQUAL "STM32F7")
    set(PLATFORM_DEFINES
        STM32F7xx              # Board family
        ARM_MATH_CM7           # Architecture
        TARGET_BOARD_STM32F7=1 # Conflates both
    )
    # Mix architecture and board sources
    set(BOARD_SOURCES board/stm32f7/...)
endif()
```

**Problems:**
1. Cannot select architecture independently from board
2. Cannot add new boards without modifying CMake logic
3. No clear mapping: "This board uses this architecture"

---

## 4. Proposed Solution

### 4.1 Design Principles

1. **Separation of Concerns**: Architecture code must not know about board details
2. **Single Responsibility**: Each layer has one clear purpose
3. **Dependency Inversion**: Higher layers depend on abstractions, not implementations
4. **Open/Closed Principle**: Easy to add new boards without modifying existing code
5. **Don't Repeat Yourself**: Share architecture code across all boards with same CPU

### 4.2 Three-Layer Model Overview

```
┌───────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                      │
│  - User's main.c                                           │
│  - Calls embedDIP API                                      │
│  - No hardware knowledge needed                            │
└───────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────┐
│                     BOARD LAYER (BSP)                      │
│  Purpose: Define "what hardware exists and how it's wired" │
│  Examples:                                                 │
│  - boards/stm32f746g_discovery/                           │
│      • Pin mappings (Camera power = PH13)                 │
│      • Memory map (8MB SDRAM at 0xC0000000)               │
│      • Peripherals (DCMI, LTDC, DMA2D)                    │
│      • Inherits: ARM_CORTEX_M7                            │
│  - boards/esp32_cam/                                       │
│      • Pin mappings (Camera I2C = GPIO26/27)              │
│      • Memory (4MB PSRAM)                                  │
│      • Inherits: XTENSA_LX6                               │
└───────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────┐
│                  ARCHITECTURE LAYER (HAL)                  │
│  Purpose: Provide "CPU-optimized implementations"          │
│  Examples:                                                 │
│  - arch/arm_cortex_m7/                                    │
│      • FFT using CMSIS-DSP                                │
│      • Memory allocation API                               │
│      • Cache management                                    │
│  - arch/xtensa_lx6/                                       │
│      • FFT using ESP-DSP                                  │
│      • PSRAM allocation                                    │
│      • Dual-core task management                           │
└───────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────┐
│                    PORTABLE LAYER                          │
│  Purpose: Generic algorithms that work everywhere          │
│  - core/image.h (data structures)                         │
│  - imgproc/filter.c (convolution math)                    │
│  - imgproc/color.c (color space conversions)              │
│  Status: Already clean ✅                                  │
└───────────────────────────────────────────────────────────┘
```

### 4.3 Layer Responsibilities

#### Layer 1: Portable (core/, imgproc/)

**Responsibility:** Provide generic, platform-independent functionality

**Rules:**
- ✅ Use only standard C/C++ library
- ✅ Define abstract interfaces
- ❌ No hardware-specific code
- ❌ No CPU-specific optimizations

**Example:**
```c
// core/memory_manager.h (abstraction)
void* memory_alloc(size_t size);     // Abstract interface
void memory_free(void* ptr);
```

#### Layer 2: Architecture (arch/)

**Responsibility:** Implement CPU-specific optimizations

**Rules:**
- ✅ Use CPU instruction set (NEON, DSP extensions)
- ✅ Use architecture-specific libraries (CMSIS-DSP, ESP-DSP)
- ✅ Can vary by CPU model (Cortex-M4 vs M7)
- ❌ No board-specific addresses or pins
- ❌ No peripheral initialization

**Example:**
```c
// arch/arm_cortex_m7/arch_fft.c
#include "arm_math.h"

int arch_fft_2d(const float *input, float *output, int width, int height) {
    // Uses Cortex-M7 DSP instructions
    for (int row = 0; row < height; row++) {
        arm_cfft_f32(&arm_cfft_sR_f32_len256,
                     output + row * width * 2, 0, 1);
    }
    return 0;
}
```

#### Layer 3: Board (boards/)

**Responsibility:** Define hardware configuration

**Rules:**
- ✅ Define pin mappings
- ✅ Define memory layout (addresses, sizes)
- ✅ Configure peripherals
- ✅ Specify which architecture to use
- ❌ No algorithm implementations
- ❌ No CPU-specific optimizations

**Example:**
```c
// boards/stm32f746g_discovery/board_config.h
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Specify architecture (this pulls in arch layer automatically)
#define ARCH_ARM_CORTEX_M7 1

// Board identification
#define BOARD_NAME "STM32F746G-Discovery"

// Memory configuration
#define BOARD_SDRAM_BASE 0xC0000000
#define BOARD_SDRAM_SIZE (8 * 1024 * 1024)  // 8MB

// Pin mappings
#define BOARD_CAMERA_PWR_PIN GPIO_PIN_13
#define BOARD_CAMERA_PWR_PORT GPIOH

// Available peripherals
#define BOARD_HAS_CAMERA_OV5640 1
#define BOARD_HAS_LCD_RK043FN48H 1

#endif
```

---

## 5. Three-Layer Architecture Model

### 5.1 Directory Structure

```
embedDIP/
│
├── core/                          # ✅ Portable Layer (no changes)
│   ├── image.h                    # Image data structures
│   ├── error.c                    # Error handling
│   └── memory_manager.h           # Abstract memory interface
│
├── imgproc/                       # ✅ Portable Layer (no changes)
│   ├── color.c                    # Color conversions (pure C)
│   ├── filter.c                   # Convolution algorithms
│   ├── histogram.c                # Histogram operations
│   └── pixel.c                    # Pixel manipulation
│
├── arch/                          # 🆕 Architecture Layer
│   │
│   ├── arch.h                     # Common interface for all architectures
│   │
│   ├── arm_cortex_m7/             # ARM Cortex-M7 implementation
│   │   ├── arch_config.h          # Architecture capabilities
│   │   ├── arch_fft.c             # FFT using CMSIS-DSP
│   │   ├── arch_memory.c          # Memory allocation API
│   │   └── arch_init.c            # CPU initialization
│   │
│   ├── arm_cortex_m4/             # ARM Cortex-M4 implementation
│   │   ├── arch_config.h
│   │   ├── arch_fft.c             # FFT (single-precision FPU)
│   │   └── arch_memory.c
│   │
│   ├── xtensa_lx6/                # Xtensa LX6 (ESP32)
│   │   ├── arch_config.h
│   │   ├── arch_fft.cpp           # FFT using ESP-DSP
│   │   ├── arch_memory.cpp        # PSRAM allocation
│   │   └── arch_init.cpp
│   │
│   ├── xtensa_lx7/                # Xtensa LX7 (ESP32-S3)
│   │   ├── arch_config.h
│   │   ├── arch_fft.cpp           # FFT with AI extensions
│   │   └── arch_memory.cpp
│   │
│   └── host/                      # x86/x64 for testing
│       ├── arch_config.h
│       ├── arch_fft.c             # Software FFT (FFTW or KissFFT)
│       └── arch_memory.c          # malloc/free wrapper
│
├── boards/                        # 🆕 Board Support Packages
│   │
│   ├── stm32f746g_discovery/      # STM32F746G-Discovery board
│   │   ├── board_config.h         # Hardware configuration
│   │   ├── board_init.c           # Board initialization
│   │   ├── board_memory_map.h     # Memory addresses
│   │   └── README.md              # Board documentation
│   │
│   ├── stm32f767zi_nucleo/        # STM32F767ZI-Nucleo board
│   │   ├── board_config.h         # Different pins, no SDRAM
│   │   ├── board_init.c
│   │   └── README.md
│   │
│   ├── esp32_cam/                 # ESP32-CAM module
│   │   ├── board_config.h
│   │   ├── board_init.cpp
│   │   └── README.md
│   │
│   ├── esp32_wrover_kit/          # ESP32-WROVER-KIT
│   │   ├── board_config.h
│   │   ├── board_init.cpp
│   │   └── README.md
│   │
│   └── host/                      # PC/Linux for testing
│       ├── board_config.h
│       ├── board_init.c
│       └── README.md
│
├── device/                        # Device drivers (can be shared)
│   ├── camera/
│   │   ├── camera.h               # Generic camera interface
│   │   ├── ov5640/
│   │   │   ├── ov5640.c           # Generic OV5640 driver
│   │   │   ├── ov5640.h
│   │   │   ├── ov5640_stm32.c     # STM32 DCMI interface
│   │   │   └── ov5640_regs.h      # Register definitions
│   │   └── ov2640/
│   │       ├── ov2640.c           # Generic OV2640 driver
│   │       ├── ov2640.h
│   │       └── ov2640_esp32.cpp   # ESP32 I2C interface
│   │
│   ├── display/
│   │   ├── display.h              # Generic display interface
│   │   ├── rk043fn48h/            # RK043FN48H LCD (STM32)
│   │   │   ├── rk043fn48h.c
│   │   │   └── rk043fn48h.h
│   │   └── ili9341/               # ILI9341 LCD (SPI - many boards)
│   │       ├── ili9341.c
│   │       └── ili9341.h
│   │
│   └── serial/
│       ├── serial.h               # Generic UART interface
│       ├── stm32_uart/
│       │   └── stm32_uart.c
│       └── esp32_uart/
│           └── esp32_uart.cpp
│
├── examples/                      # Example projects
│   ├── fft_demo/
│   ├── camera_capture/
│   └── ...
│
├── tests/                         # Unit tests
│   ├── test_core/
│   ├── test_imgproc/
│   └── ...
│
├── docs/                          # Documentation
│   ├── ARCHITECTURE_REFACTORING.md  # This document
│   ├── ARCHITECTURE_REFACTORING.tex # LaTeX version
│   ├── PORTING_GUIDE.md
│   └── API_REFERENCE.md
│
├── CMakeLists.txt                 # Updated build system
├── embedDIP.h                     # Main library header
└── README.md
```

### 5.2 Key Interfaces

#### 5.2.1 Architecture Interface (arch/arch.h)

This header defines the common interface that all architectures must implement:

```c
#ifndef EMBEDDIP_ARCH_H
#define EMBEDDIP_ARCH_H

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Memory Management Interface
// ============================================================================

/**
 * @brief Initialize architecture-specific memory subsystem
 *
 * Called once during board initialization. May configure:
 * - External RAM (SDRAM, PSRAM)
 * - Memory pools
 * - Cache settings
 */
void arch_memory_init(void);

/**
 * @brief Allocate memory
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* arch_memory_alloc(size_t size);

/**
 * @brief Free previously allocated memory
 * @param ptr Pointer to memory block
 */
void arch_memory_free(void* ptr);

/**
 * @brief Reallocate memory block
 * @param ptr Existing pointer (or NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* arch_memory_realloc(void* ptr, size_t new_size);

// ============================================================================
// FFT Interface
// ============================================================================

/**
 * @brief Perform 2D Fast Fourier Transform
 *
 * @param input Input buffer (real values)
 * @param output Output buffer (complex: real, imag interleaved)
 * @param width Image width (must be power of 2)
 * @param height Image height (must be power of 2)
 * @return 0 on success, negative error code on failure
 */
int arch_fft_2d(const float* input, float* output, int width, int height);

/**
 * @brief Perform 2D Inverse Fast Fourier Transform
 *
 * @param input Input buffer (complex: real, imag interleaved)
 * @param output Output buffer (real values)
 * @param width Image width (must be power of 2)
 * @param height Image height (must be power of 2)
 * @return 0 on success, negative error code on failure
 */
int arch_ifft_2d(const float* input, float* output, int width, int height);

// ============================================================================
// Initialization Interface
// ============================================================================

/**
 * @brief Initialize architecture-specific features
 *
 * Called during system startup. May configure:
 * - FPU settings
 * - Cache policies
 * - Clock speeds
 */
void arch_init(void);

// ============================================================================
// Architecture Capabilities (Compile-time)
// ============================================================================

// These are defined in arch_config.h for each architecture
// #define ARCH_HAS_FPU 1
// #define ARCH_HAS_DSP 1
// #define ARCH_HAS_CACHE 1
// #define ARCH_MAX_FFT_SIZE 4096

#endif // EMBEDDIP_ARCH_H
```

#### 5.2.2 Board Configuration Interface (boards/board_config.h)

Each board defines its configuration:

```c
// Example: boards/stm32f746g_discovery/board_config.h

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================================
// Architecture Selection (Mandatory)
// ============================================================================
#define ARCH_ARM_CORTEX_M7 1
#include "arch/arm_cortex_m7/arch_config.h"

// ============================================================================
// Board Identification
// ============================================================================
#define BOARD_NAME "STM32F746G-Discovery"
#define BOARD_VENDOR "STMicroelectronics"
#define BOARD_FAMILY "STM32F7"

// ============================================================================
// Memory Configuration
// ============================================================================
#define BOARD_FLASH_BASE 0x08000000
#define BOARD_FLASH_SIZE (1024 * 1024)  // 1MB

#define BOARD_SRAM_BASE 0x20000000
#define BOARD_SRAM_SIZE (320 * 1024)    // 320KB

#define BOARD_SDRAM_BASE 0xC0000000
#define BOARD_SDRAM_SIZE (8 * 1024 * 1024)  // 8MB external SDRAM

// Frame buffer allocation
#define BOARD_FRAMEBUFFER_BASE BOARD_SDRAM_BASE
#define BOARD_FRAMEBUFFER_SIZE (480 * 272 * 4)  // ARGB8888

// Dynamic memory pool (after framebuffer)
#define BOARD_MEMORY_POOL_BASE (BOARD_FRAMEBUFFER_BASE + BOARD_FRAMEBUFFER_SIZE)
#define BOARD_MEMORY_POOL_SIZE (BOARD_SDRAM_SIZE - BOARD_FRAMEBUFFER_SIZE)

// ============================================================================
// Peripheral Availability
// ============================================================================
#define BOARD_HAS_CAMERA 1
#define BOARD_CAMERA_TYPE "OV5640"
#define BOARD_CAMERA_INTERFACE "DCMI"

#define BOARD_HAS_LCD 1
#define BOARD_LCD_TYPE "RK043FN48H"
#define BOARD_LCD_WIDTH 480
#define BOARD_LCD_HEIGHT 272
#define BOARD_LCD_INTERFACE "LTDC"

#define BOARD_HAS_UART 1
#define BOARD_UART_DEFAULT_PORT 1  // USART1

// ============================================================================
// Pin Mappings
// ============================================================================
// Camera
#define BOARD_CAMERA_PWR_PIN GPIO_PIN_13
#define BOARD_CAMERA_PWR_PORT GPIOH
#define BOARD_CAMERA_RST_PIN GPIO_PIN_14
#define BOARD_CAMERA_RST_PORT GPIOH

// UART
#define BOARD_UART_TX_PIN GPIO_PIN_9
#define BOARD_UART_TX_PORT GPIOA
#define BOARD_UART_RX_PIN GPIO_PIN_10
#define BOARD_UART_RX_PORT GPIOA

// ============================================================================
// Clock Configuration
// ============================================================================
#define BOARD_CPU_CLOCK_HZ 216000000  // 216 MHz
#define BOARD_APB1_CLOCK_HZ 54000000  // 54 MHz
#define BOARD_APB2_CLOCK_HZ 108000000 // 108 MHz

#endif // BOARD_CONFIG_H
```

---

## 6. Implementation Details

### 6.1 Architecture Layer Implementation

#### 6.1.1 ARM Cortex-M7 FFT Implementation

**File:** `arch/arm_cortex_m7/arch_fft.c`

```c
#include "arch/arch.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include <string.h>

// Validate FFT size (must be power of 2 and square)
static int is_valid_fft_size(int width, int height) {
    if (width != height) return 0;
    if ((width & (width - 1)) != 0) return 0;  // Not power of 2
    if (width < 16 || width > 4096) return 0;
    return 1;
}

// Select appropriate CMSIS-DSP FFT instance based on size
static const arm_cfft_instance_f32* get_fft_instance(int size) {
    switch(size) {
        case 16:   return &arm_cfft_sR_f32_len16;
        case 32:   return &arm_cfft_sR_f32_len32;
        case 64:   return &arm_cfft_sR_f32_len64;
        case 128:  return &arm_cfft_sR_f32_len128;
        case 256:  return &arm_cfft_sR_f32_len256;
        case 512:  return &arm_cfft_sR_f32_len512;
        case 1024: return &arm_cfft_sR_f32_len1024;
        case 2048: return &arm_cfft_sR_f32_len2048;
        case 4096: return &arm_cfft_sR_f32_len4096;
        default:   return NULL;
    }
}

int arch_fft_2d(const float* input, float* output, int width, int height) {
    // Validate input
    if (!input || !output) return -1;
    if (!is_valid_fft_size(width, height)) return -2;

    const arm_cfft_instance_f32* fft_inst = get_fft_instance(width);
    if (!fft_inst) return -3;

    int N = width;

    // Allocate temporary buffer for transpose
    float* temp = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp) return -4;

    // Convert input to complex (interleaved real, imag)
    for (int i = 0; i < N * N; i++) {
        output[2 * i] = input[i];      // Real part
        output[2 * i + 1] = 0.0f;      // Imaginary part
    }

    // FFT on rows
    for (int row = 0; row < N; row++) {
        arm_cfft_f32(fft_inst, output + row * N * 2, 0, 1);
    }

    // Transpose: output -> temp
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            temp[dst] = output[src];
            temp[dst + 1] = output[src + 1];
        }
    }

    // FFT on columns (now rows after transpose)
    for (int row = 0; row < N; row++) {
        arm_cfft_f32(fft_inst, temp + row * N * 2, 0, 1);
    }

    // Transpose back: temp -> output
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            output[dst] = temp[src];
            output[dst + 1] = temp[src + 1];
        }
    }

    arch_memory_free(temp);
    return 0;
}

int arch_ifft_2d(const float* input, float* output, int width, int height) {
    // Validate input
    if (!input || !output) return -1;
    if (!is_valid_fft_size(width, height)) return -2;

    const arm_cfft_instance_f32* fft_inst = get_fft_instance(width);
    if (!fft_inst) return -3;

    int N = width;

    // Allocate working buffers
    float* temp = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp) return -4;

    // Copy input to temp
    memcpy(temp, input, N * N * 2 * sizeof(float));

    // Inverse FFT on rows
    for (int row = 0; row < N; row++) {
        arm_cfft_f32(fft_inst, temp + row * N * 2, 1, 1);  // ifft flag = 1
    }

    // Transpose
    float* temp2 = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp2) {
        arch_memory_free(temp);
        return -4;
    }

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            temp2[dst] = temp[src];
            temp2[dst + 1] = temp[src + 1];
        }
    }

    // Inverse FFT on columns
    for (int row = 0; row < N; row++) {
        arm_cfft_f32(fft_inst, temp2 + row * N * 2, 1, 1);
    }

    // Transpose back and extract real part
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (x * N + y);
            output[y * N + x] = temp2[src];  // Take real part only
        }
    }

    arch_memory_free(temp);
    arch_memory_free(temp2);
    return 0;
}
```

**Key Points:**
- Uses ARM CMSIS-DSP library (architecture-specific)
- No board-specific code (no pin mappings, no memory addresses)
- Works on ANY ARM Cortex-M7 board (STM32F7, STM32H7, NXP i.MX RT)
- Can be tested independently from board hardware

#### 6.1.2 ARM Cortex-M7 Memory Implementation

**File:** `arch/arm_cortex_m7/arch_memory.c`

```c
#include "arch/arch.h"
#include "board_config.h"  // Gets memory addresses from board
#include <string.h>

// Memory block header for allocator
typedef struct MemoryBlock {
    size_t size;
    struct MemoryBlock* next;
    int is_free;
} MemoryBlock;

#define ALIGN4(s) (((s) + 3) & ~3)
#define BLOCK_SIZE sizeof(MemoryBlock)

static MemoryBlock* free_list = NULL;
static int initialized = 0;

void arch_memory_init(void) {
    if (initialized) return;

    // Board provides base address and size
    uint8_t* pool_base = (uint8_t*)BOARD_MEMORY_POOL_BASE;
    size_t pool_size = BOARD_MEMORY_POOL_SIZE;

    // Initialize free list
    free_list = (MemoryBlock*)pool_base;
    free_list->size = pool_size - BLOCK_SIZE;
    free_list->next = NULL;
    free_list->is_free = 1;

    initialized = 1;
}

void* arch_memory_alloc(size_t size) {
    if (!initialized) arch_memory_init();

    size = ALIGN4(size);
    MemoryBlock* curr = free_list;

    // First-fit allocation
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            // Split block if large enough
            if (curr->size >= size + BLOCK_SIZE + 32) {
                MemoryBlock* new_block =
                    (MemoryBlock*)((uint8_t*)curr + BLOCK_SIZE + size);
                new_block->size = curr->size - size - BLOCK_SIZE;
                new_block->next = curr->next;
                new_block->is_free = 1;

                curr->next = new_block;
                curr->size = size;
            }

            curr->is_free = 0;
            return (void*)((uint8_t*)curr + BLOCK_SIZE);
        }
        curr = curr->next;
    }

    return NULL;  // Out of memory
}

void arch_memory_free(void* ptr) {
    if (!ptr) return;

    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - BLOCK_SIZE);
    block->is_free = 1;

    // Coalesce adjacent free blocks
    MemoryBlock* curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += BLOCK_SIZE + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void* arch_memory_realloc(void* ptr, size_t new_size) {
    if (!ptr) return arch_memory_alloc(new_size);
    if (new_size == 0) {
        arch_memory_free(ptr);
        return NULL;
    }

    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - BLOCK_SIZE);
    if (block->size >= new_size) return ptr;  // Already big enough

    // Allocate new block and copy
    void* new_ptr = arch_memory_alloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        arch_memory_free(ptr);
    }
    return new_ptr;
}
```

**Key Points:**
- Gets memory pool address from board config (`BOARD_MEMORY_POOL_BASE`)
- Implements generic memory allocator algorithm
- No hardcoded addresses (board-independent)

#### 6.1.3 Xtensa LX6 FFT Implementation

**File:** `arch/xtensa_lx6/arch_fft.cpp`

```cpp
#include "arch/arch.h"
#include "esp_dsp.h"
#include <Arduino.h>

extern "C" {

static bool is_valid_fft_size(int width, int height) {
    if (width != height) return false;
    if ((width & (width - 1)) != 0) return false;  // Not power of 2
    if (width < 16 || width > 4096) return false;
    return true;
}

int arch_fft_2d(const float* input, float* output, int width, int height) {
    if (!input || !output) return -1;
    if (!is_valid_fft_size(width, height)) return -2;

    int N = width;

    // Initialize ESP-DSP
    dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    // Allocate temp buffer
    float* temp = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp) return -4;

    // Convert to complex
    for (int i = 0; i < N * N; i++) {
        output[2 * i] = input[i];
        output[2 * i + 1] = 0.0f;
    }

    // FFT on rows
    for (int row = 0; row < N; row++) {
        dsps_fft2r_fc32(output + row * N * 2, N);
        dsps_bit_rev_fc32(output + row * N * 2, N);
    }

    // Transpose
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            temp[dst] = output[src];
            temp[dst + 1] = output[src + 1];
        }
    }

    // FFT on columns
    for (int row = 0; row < N; row++) {
        dsps_fft2r_fc32(temp + row * N * 2, N);
        dsps_bit_rev_fc32(temp + row * N * 2, N);
    }

    // Transpose back
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            output[dst] = temp[src];
            output[dst + 1] = temp[src + 1];
        }
    }

    arch_memory_free(temp);
    return 0;
}

int arch_ifft_2d(const float* input, float* output, int width, int height) {
    // Similar implementation using dsps_fft2r_fc32 with inverse flag
    // ... (implementation omitted for brevity)
    return 0;
}

} // extern "C"
```

**Key Points:**
- Uses ESP-DSP library (Xtensa-specific)
- Same interface as ARM version (`arch_fft_2d`)
- Application code doesn't need to know which architecture is used

### 6.2 Board Layer Implementation

#### 6.2.1 STM32F746G-Discovery Board Init

**File:** `boards/stm32f746g_discovery/board_init.c`

```c
#include "board_config.h"
#include "arch/arch.h"
#include "stm32f7xx_hal.h"

// External peripheral handles (generated by CubeMX)
extern DCMI_HandleTypeDef hdcmi;
extern LTDC_HandleTypeDef hltdc;
extern DMA2D_HandleTypeDef hdma2d;
extern UART_HandleTypeDef huart1;

void board_init(void) {
    // HAL initialization
    HAL_Init();

    // Configure system clock to 216 MHz
    SystemClock_Config();

    // Initialize architecture layer
    arch_init();
    arch_memory_init();

    // Initialize board-specific peripherals
    board_camera_power_init();
    board_lcd_init();
    board_uart_init();
}

void board_camera_power_init(void) {
    GPIO_InitTypeDef gpio_init = {0};

    // Enable clock
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // Configure camera power pin
    gpio_init.Pin = BOARD_CAMERA_PWR_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(BOARD_CAMERA_PWR_PORT, &gpio_init);

    // Power off initially
    HAL_GPIO_WritePin(BOARD_CAMERA_PWR_PORT, BOARD_CAMERA_PWR_PIN, GPIO_PIN_RESET);
}

void board_lcd_init(void) {
    // Initialize LTDC for RGB interface
    // Board-specific pin configuration
    // ...
}

void board_uart_init(void) {
    // Configure UART pins from board_config.h
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio_init.Pin = BOARD_UART_TX_PIN | BOARD_UART_RX_PIN;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // Configure UART parameters
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}
```

**Key Points:**
- All pin mappings come from `board_config.h`
- No algorithm implementations (just hardware setup)
- Calls architecture layer for CPU initialization

#### 6.2.2 ESP32-CAM Board Init

**File:** `boards/esp32_cam/board_init.cpp`

```cpp
#include "board_config.h"
#include "arch/arch.h"
#include <Arduino.h>
#include <Wire.h>

extern "C" {

void board_init(void) {
    // Initialize Serial
    Serial.begin(115200);
    Serial.println("[Board] ESP32-CAM initializing...");

    // Initialize architecture layer
    arch_init();
    arch_memory_init();

    // Initialize I2C for camera
    Wire.begin(BOARD_CAMERA_I2C_SDA, BOARD_CAMERA_I2C_SCL);
    Wire.setClock(400000);  // 400kHz

    // Initialize camera power pins
    pinMode(BOARD_CAMERA_PWDN, OUTPUT);
    pinMode(BOARD_CAMERA_RESET, OUTPUT);

    // Power up camera
    digitalWrite(BOARD_CAMERA_PWDN, LOW);   // Enable camera
    digitalWrite(BOARD_CAMERA_RESET, HIGH); // Release reset
    delay(100);

    Serial.println("[Board] ESP32-CAM initialized");
}

void board_uart_init(void) {
    // Already initialized in board_init()
}

} // extern "C"
```

**Key Points:**
- Different hardware (I2C vs DCMI for camera)
- Different pins, different initialization
- Still calls same architecture layer (`arch_init()`)

### 6.3 Updated Build System

#### 6.3.1 Top-Level CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)

# ============================================================================
# Board Selection (User configures this)
# ============================================================================
set(EMBEDDIP_BOARD "STM32F746G_DISCOVERY" CACHE STRING
    "Target board (determines architecture automatically)")

set_property(CACHE EMBEDDIP_BOARD PROPERTY STRINGS
    "STM32F746G_DISCOVERY"
    "STM32F767ZI_NUCLEO"
    "ESP32_CAM"
    "ESP32_WROVER_KIT"
    "HOST"
)

# ============================================================================
# Board Configuration (Maps board -> architecture + sources)
# ============================================================================
include(cmake/BoardConfigs.cmake)

# ============================================================================
# Project Definition
# ============================================================================
project(embedDIP VERSION 1.0.0 LANGUAGES C CXX)

# ============================================================================
# Core Library (Portable - always included)
# ============================================================================
set(CORE_SOURCES
    core/error.c
    core/image.h
    core/memory_manager.h
)

set(IMGPROC_SOURCES
    imgproc/color.c
    imgproc/filter.c
    imgproc/histogram.c
    imgproc/pixel.c
)

# ============================================================================
# Create Library
# ============================================================================
add_library(embedDIP STATIC
    ${CORE_SOURCES}
    ${IMGPROC_SOURCES}
    ${ARCH_SOURCES}      # From board config
    ${BOARD_SOURCES}     # From board config
    ${DEVICE_SOURCES}    # From board config
)

# ============================================================================
# Include Directories
# ============================================================================
target_include_directories(embedDIP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/imgproc
    ${CMAKE_CURRENT_SOURCE_DIR}/arch
    ${ARCH_INCLUDE_DIRS}    # From board config
    ${BOARD_INCLUDE_DIRS}   # From board config
    ${DEVICE_INCLUDE_DIRS}  # From board config
)

# ============================================================================
# Compile Definitions
# ============================================================================
target_compile_definitions(embedDIP PUBLIC
    ${ARCH_DEFINES}         # From board config
    ${BOARD_DEFINES}        # From board config
)

# ============================================================================
# Link Libraries
# ============================================================================
target_link_libraries(embedDIP PUBLIC
    ${ARCH_LIBRARIES}       # From board config (CMSIS-DSP, ESP-DSP, etc.)
)

# ============================================================================
# Configuration Summary
# ============================================================================
message(STATUS "===================================")
message(STATUS "embedDIP Configuration")
message(STATUS "===================================")
message(STATUS "Board:         ${EMBEDDIP_BOARD}")
message(STATUS "Architecture:  ${EMBEDDIP_ARCH}")
message(STATUS "CPU:           ${EMBEDDIP_CPU}")
message(STATUS "Toolchain:     ${CMAKE_C_COMPILER_ID}")
message(STATUS "===================================")
```

#### 6.3.2 Board Configuration File

**File:** `cmake/BoardConfigs.cmake`

```cmake
# ============================================================================
# Board Configuration Mappings
# ============================================================================

if(EMBEDDIP_BOARD STREQUAL "STM32F746G_DISCOVERY")
    # Architecture
    set(EMBEDDIP_ARCH "ARM_CORTEX_M7")
    set(EMBEDDIP_CPU "cortex-m7")

    # Architecture sources
    set(ARCH_SOURCES
        arch/arm_cortex_m7/arch_fft.c
        arch/arm_cortex_m7/arch_memory.c
        arch/arm_cortex_m7/arch_init.c
    )

    set(ARCH_INCLUDE_DIRS
        arch/arm_cortex_m7
    )

    set(ARCH_DEFINES
        ARCH_ARM_CORTEX_M7=1
        ARM_MATH_CM7
        __FPU_PRESENT=1
    )

    set(ARCH_LIBRARIES
        # Link with CMSIS-DSP
        ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Lib/libarm_cortexM7lfdp_math.a
    )

    # Board sources
    set(BOARD_SOURCES
        boards/stm32f746g_discovery/board_init.c
    )

    set(BOARD_INCLUDE_DIRS
        boards/stm32f746g_discovery
        ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32F7xx_HAL_Driver/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32F7xx/Include
    )

    set(BOARD_DEFINES
        BOARD_STM32F746G_DISCOVERY=1
        STM32F746xx
        USE_HAL_DRIVER
    )

    # Device sources (peripherals on this board)
    set(DEVICE_SOURCES
        device/camera/ov5640/ov5640.c
        device/camera/ov5640/ov5640_stm32.c
        device/display/rk043fn48h/rk043fn48h.c
        device/serial/stm32_uart/stm32_uart.c
    )

    set(DEVICE_INCLUDE_DIRS
        device/camera/ov5640
        device/display/rk043fn48h
        device/serial/stm32_uart
    )

elseif(EMBEDDIP_BOARD STREQUAL "ESP32_CAM")
    # Architecture
    set(EMBEDDIP_ARCH "XTENSA_LX6")
    set(EMBEDDIP_CPU "xtensa-lx6")

    # Architecture sources
    set(ARCH_SOURCES
        arch/xtensa_lx6/arch_fft.cpp
        arch/xtensa_lx6/arch_memory.cpp
        arch/xtensa_lx6/arch_init.cpp
    )

    set(ARCH_INCLUDE_DIRS
        arch/xtensa_lx6
    )

    set(ARCH_DEFINES
        ARCH_XTENSA_LX6=1
        ARDUINO_ARCH_ESP32
    )

    set(ARCH_LIBRARIES
        # ESP-DSP linked via Arduino framework
    )

    # Board sources
    set(BOARD_SOURCES
        boards/esp32_cam/board_init.cpp
    )

    set(BOARD_INCLUDE_DIRS
        boards/esp32_cam
    )

    set(BOARD_DEFINES
        BOARD_ESP32_CAM=1
    )

    # Device sources
    set(DEVICE_SOURCES
        device/camera/ov2640/ov2640.c
        device/camera/ov2640/ov2640_esp32.cpp
        device/serial/esp32_uart/esp32_uart.cpp
    )

    set(DEVICE_INCLUDE_DIRS
        device/camera/ov2640
        device/serial/esp32_uart
    )

elseif(EMBEDDIP_BOARD STREQUAL "HOST")
    # Architecture (native x86/x64)
    set(EMBEDDIP_ARCH "HOST")
    set(EMBEDDIP_CPU "native")

    set(ARCH_SOURCES
        arch/host/arch_fft.c
        arch/host/arch_memory.c
        arch/host/arch_init.c
    )

    set(ARCH_INCLUDE_DIRS
        arch/host
    )

    set(ARCH_DEFINES
        ARCH_HOST=1
    )

    set(ARCH_LIBRARIES
        m  # Math library
    )

    # Board sources (minimal for host)
    set(BOARD_SOURCES
        boards/host/board_init.c
    )

    set(BOARD_INCLUDE_DIRS
        boards/host
    )

    set(BOARD_DEFINES
        BOARD_HOST=1
    )

    # Device sources (file-based I/O)
    set(DEVICE_SOURCES
        device/camera/host/host_camera.c
        device/display/host/host_display.c
        device/serial/host/host_uart.c
    )

    set(DEVICE_INCLUDE_DIRS
        device/camera/host
        device/display/host
        device/serial/host
    )

else()
    message(FATAL_ERROR "Unsupported board: ${EMBEDDIP_BOARD}")
endif()
```

---

## 7. Migration Guide

### 7.1 Migration Steps

#### Phase 1: Preparation (Week 1)

**Step 1.1: Create New Directory Structure**
```bash
cd embedDIP
mkdir -p arch/{arm_cortex_m7,arm_cortex_m4,xtensa_lx6,host}
mkdir -p boards/{stm32f746g_discovery,esp32_cam,host}
```

**Step 1.2: Create Interface Headers**
- Create `arch/arch.h` with common interface
- Create template `board_config.h` files

**Step 1.3: Backup Current Code**
```bash
git checkout -b feature/architecture-refactoring
git commit -am "Checkpoint before refactoring"
```

#### Phase 2: Extract Architecture Layer (Week 2)

**Step 2.1: Move ARM Cortex-M7 FFT Code**
```bash
# Copy and adapt
cp board/stm32f7/board_stm32f7_fft.c arch/arm_cortex_m7/arch_fft.c
```

**Changes needed in `arch_fft.c`:**
1. Remove `#ifdef TARGET_BOARD_STM32F7`
2. Replace with `#include "arch/arch.h"`
3. Rename functions to `arch_fft_2d()`, `arch_ifft_2d()`
4. Remove any board-specific code (memory addresses, pin configs)

**Step 2.2: Move ARM Cortex-M7 Memory Code**
```bash
cp board/stm32f7/board_stm32f7_memory.c arch/arm_cortex_m7/arch_memory.c
```

**Changes needed:**
1. Replace hardcoded addresses with `BOARD_MEMORY_POOL_BASE` from board config
2. Rename functions to `arch_memory_*()` pattern

**Step 2.3: Repeat for Xtensa LX6**
```bash
cp board/esp32/board_esp32_fft.cpp arch/xtensa_lx6/arch_fft.cpp
cp board/esp32/board_esp32_memory.cpp arch/xtensa_lx6/arch_memory.cpp
```

**Step 2.4: Test Architecture Layer Independently**
- Create simple test program that calls `arch_fft_2d()`
- Verify it compiles and links correctly

#### Phase 3: Create Board Configurations (Week 3)

**Step 3.1: Extract Board-Specific Defines**

From `board/stm32f7/configs.h`, extract to `boards/stm32f746g_discovery/board_config.h`:
```c
// Before (mixed):
#define FRAME_BUFFER 0xC0000000
#define QVGA_RES_X 320

// After (board config):
#define BOARD_FRAMEBUFFER_BASE 0xC0000000
#define BOARD_CAMERA_WIDTH 320
#define BOARD_CAMERA_HEIGHT 240
```

**Step 3.2: Create Board Init Functions**

Move hardware initialization from main.c to `boards/.../board_init.c`

**Step 3.3: Update Device Drivers**

Modify device drivers to use board config:
```c
// Before:
#define CAMERA_PWR_PIN GPIO_PIN_13

// After:
#include "board_config.h"
gpio_init.Pin = BOARD_CAMERA_PWR_PIN;  // From board config
```

#### Phase 4: Update Build System (Week 4)

**Step 4.1: Create cmake/BoardConfigs.cmake**

Map each board to its architecture and sources.

**Step 4.2: Update CMakeLists.txt**

Replace platform selection with board selection:
```cmake
# Before:
set(EMBEDDIP_TARGET_PLATFORM "STM32F7" ...)

# After:
set(EMBEDDIP_BOARD "STM32F746G_DISCOVERY" ...)
```

**Step 4.3: Test Build for Each Board**
```bash
# Test STM32F746G-Discovery
cmake -B build_stm32 -DEMBEDDIP_BOARD=STM32F746G_DISCOVERY
cmake --build build_stm32

# Test ESP32-CAM
cmake -B build_esp32 -DEMBEDDIP_BOARD=ESP32_CAM
cmake --build build_esp32

# Test HOST
cmake -B build_host -DEMBEDDIP_BOARD=HOST
cmake --build build_host
```

#### Phase 5: Update Workflows (Week 5)

**Step 5.1: Create Build Matrix by Board**
```yaml
strategy:
  matrix:
    board:
      - STM32F746G_DISCOVERY
      - ESP32_CAM
      - HOST
```

**Step 5.2: Group by Architecture**
```yaml
architecture-tests:
  strategy:
    matrix:
      arch: [ARM_CORTEX_M7, XTENSA_LX6, HOST]
```

#### Phase 6: Testing and Validation (Week 6)

**Step 6.1: Unit Tests**
- Test each architecture layer independently
- Mock board configurations

**Step 6.2: Integration Tests**
- Test on real hardware for each board
- Verify FFT produces same results as before

**Step 6.3: Regression Tests**
- Run full test suite on all boards
- Compare results with pre-refactoring baseline

#### Phase 7: Documentation and Cleanup (Week 7)

**Step 7.1: Update Documentation**
- Update README with new board selection
- Create porting guide for new boards
- Document architecture interfaces

**Step 7.2: Remove Old Code**
```bash
# After verification, remove old structure
rm -rf board/stm32f7/board_stm32f7_fft.c
rm -rf board/esp32/board_esp32_fft.cpp
```

**Step 7.3: Update Examples**
- Update example projects to use new board configs

### 7.2 Adding a New Board

After refactoring, adding a new board is straightforward:

#### Example: Adding STM32F767ZI-Nucleo

**Step 1: Create Board Directory**
```bash
mkdir -p boards/stm32f767zi_nucleo
```

**Step 2: Create board_config.h**
```c
// boards/stm32f767zi_nucleo/board_config.h
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Use existing ARM Cortex-M7 architecture
#define ARCH_ARM_CORTEX_M7 1
#include "arch/arm_cortex_m7/arch_config.h"

// Board-specific config
#define BOARD_NAME "STM32F767ZI-Nucleo"
#define BOARD_SRAM_BASE 0x20000000
#define BOARD_SRAM_SIZE (512 * 1024)  // 512KB internal SRAM
// No external SDRAM on this board

#define BOARD_HAS_CAMERA 0  // No camera on Nucleo
#define BOARD_HAS_LCD 0     // No LCD on Nucleo
#define BOARD_HAS_UART 1

#endif
```

**Step 3: Create board_init.c**
```c
#include "board_config.h"
#include "arch/arch.h"

void board_init(void) {
    HAL_Init();
    SystemClock_Config();
    arch_init();
    arch_memory_init();
}
```

**Step 4: Add to CMake**
```cmake
# In cmake/BoardConfigs.cmake
elseif(EMBEDDIP_BOARD STREQUAL "STM32F767ZI_NUCLEO")
    set(EMBEDDIP_ARCH "ARM_CORTEX_M7")  # Reuses existing arch!
    set(ARCH_SOURCES
        arch/arm_cortex_m7/arch_fft.c      # Already exists
        arch/arm_cortex_m7/arch_memory.c   # Already exists
        arch/arm_cortex_m7/arch_init.c     # Already exists
    )
    set(BOARD_SOURCES
        boards/stm32f767zi_nucleo/board_init.c  # Only new file!
    )
    # ... rest of config
endif()
```

**Step 5: Build and Test**
```bash
cmake -B build -DEMBEDDIP_BOARD=STM32F767ZI_NUCLEO
cmake --build build
```

**Total new code required:** ~100 lines (board_config.h + board_init.c)
**Code reused:** All ARM Cortex-M7 FFT and memory code (~1000+ lines)

---

## 8. Benefits and Trade-offs

### 8.1 Benefits

#### 8.1.1 Code Reusability
**Before:**
- Adding STM32F767 requires copying 1000+ lines of FFT code
- Bug fixes must be applied to multiple copies

**After:**
- Adding STM32F767 requires ~100 lines of board config
- Bug fixes in `arch/arm_cortex_m7/arch_fft.c` apply to ALL Cortex-M7 boards automatically

#### 8.1.2 Clear Separation of Concerns

**Before:**
```c
// Mixed in one file
#ifdef TARGET_BOARD_STM32F7
    #include "arm_math.h"              // Architecture
    #define SDRAM_BASE 0xC0000000      // Board
    arm_cfft_f32(...);                 // Architecture
    GPIO_PIN_13                        // Board
#endif
```

**After:**
```c
// arch/arm_cortex_m7/arch_fft.c (pure architecture)
#include "arm_math.h"
arm_cfft_f32(...);

// boards/stm32f746g_discovery/board_config.h (pure board)
#define BOARD_SDRAM_BASE 0xC0000000
#define BOARD_CAMERA_PIN GPIO_PIN_13
```

#### 8.1.3 Easier Testing

**Before:**
- Cannot test ARM FFT without STM32 hardware
- Cannot test different memory sizes easily

**After:**
- Test ARM FFT with mock board config
- Test multiple board configs with same architecture
- Unit test each layer independently

#### 8.1.4 Better CI/CD

**Before:**
```yaml
- Platform: STM32F7  # What does this mean?
- Platform: ESP32
```

**After:**
```yaml
# Test by architecture (fewer builds)
- Architecture: ARM_CORTEX_M7
  Boards: [STM32F746G_DISCOVERY, STM32F767ZI_NUCLEO, STM32H743ZI_NUCLEO]

- Architecture: XTENSA_LX6
  Boards: [ESP32_CAM, ESP32_WROVER_KIT]
```

Only need to compile architecture code once, test on multiple boards.

#### 8.1.5 Documentation and Understanding

**Before:**
- New developers confused: "Is this board-specific or CPU-specific?"
- Unclear which code can be shared

**After:**
- Clear directory structure shows exactly what each file does
- Easy to find: "Where is ARM Cortex-M7 FFT? → `arch/arm_cortex_m7/arch_fft.c`"
- Easy to add board: "Just create board config, inherit architecture"

### 8.2 Trade-offs

#### 8.2.1 More Files
**Trade-off:** More directories and files to navigate
**Mitigation:** Clear naming conventions and documentation

#### 8.2.2 Initial Refactoring Effort
**Trade-off:** ~4-6 weeks of refactoring work
**Mitigation:** Long-term benefits outweigh short-term cost

#### 8.2.3 Learning Curve
**Trade-off:** Developers must understand 3-layer model
**Mitigation:** Comprehensive documentation (this document)

#### 8.2.4 Build Complexity
**Trade-off:** CMake configuration more complex
**Mitigation:** Centralized in `BoardConfigs.cmake`, users only select board

### 8.3 Risk Analysis

| Risk | Likelihood | Impact | Mitigation |
|------|----------|--------|------------|
| Breaking existing code | High | High | Incremental migration, maintain backward compatibility during transition |
| Performance regression | Low | Medium | Benchmark before/after, verify identical assembly output |
| Build system errors | Medium | Medium | Test all boards after each change |
| Developer confusion | Medium | Low | Training, documentation, code reviews |
| Hardware testing delays | Medium | Medium | Use hardware-in-the-loop CI, maintain test boards |

---

## 9. Testing Strategy

### 9.1 Unit Testing

#### 9.1.1 Architecture Layer Tests

Test each architecture implementation independently:

```c
// tests/test_arch_fft.c
void test_arm_cortex_m7_fft(void) {
    // Mock board config
    #define BOARD_MEMORY_POOL_BASE 0x20000000
    #define BOARD_MEMORY_POOL_SIZE (512 * 1024)

    // Test FFT
    float input[256 * 256];
    float output[256 * 256 * 2];

    // Initialize with test pattern
    for (int i = 0; i < 256 * 256; i++) {
        input[i] = (float)i;
    }

    // Run FFT
    int result = arch_fft_2d(input, output, 256, 256);
    assert(result == 0);

    // Verify DC component
    float dc_real = output[0];
    float expected_dc = (256 * 256) * (256 * 256 - 1) / 2.0f;
    assert(fabs(dc_real - expected_dc) < 1e-3);
}
```

#### 9.1.2 Board Configuration Tests

Verify board configs are valid:

```c
// tests/test_board_config.c
void test_stm32f746g_discovery_config(void) {
    #include "boards/stm32f746g_discovery/board_config.h"

    // Verify architecture selected
    assert(ARCH_ARM_CORTEX_M7 == 1);

    // Verify memory ranges valid
    assert(BOARD_SDRAM_BASE == 0xC0000000);
    assert(BOARD_SDRAM_SIZE == 8 * 1024 * 1024);
    assert(BOARD_MEMORY_POOL_BASE > BOARD_FRAMEBUFFER_BASE);

    // Verify peripherals defined
    assert(BOARD_HAS_CAMERA == 1);
    assert(BOARD_HAS_LCD == 1);
}
```

### 9.2 Integration Testing

#### 9.2.1 Cross-Layer Tests

Test interaction between layers:

```c
// tests/test_integration.c
void test_fft_end_to_end(void) {
    // Initialize board (includes arch init)
    board_init();

    // Allocate image
    Image* img = createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE);

    // Fill with test pattern
    for (int i = 0; i < 256 * 256; i++) {
        img->pixels[i] = (i % 256);
    }

    // Perform FFT (uses arch layer)
    Image* fft_result = createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE);
    int result = fft(img, fft_result);

    assert(result == 0);
    assert(fft_result->log == IMAGE_DATA_COMPLEX);

    // Cleanup
    deleteImage(img);
    deleteImage(fft_result);
}
```

#### 9.2.2 Hardware-in-the-Loop Tests

For each supported board, test on real hardware:

```yaml
# .github/workflows/hil-test.yml
hardware-tests:
  strategy:
    matrix:
      board:
        - stm32f746g_discovery
        - esp32_cam
  runs-on: [self-hosted, ${{ matrix.board }}]
  steps:
    - name: Build firmware
      run: |
        cmake -B build -DEMBEDDIP_BOARD=${{ matrix.board }}
        cmake --build build

    - name: Flash to board
      run: |
        ./scripts/flash_${{ matrix.board }}.sh build/embedDIP.bin

    - name: Run hardware tests
      run: |
        python tests/hil_test.py --board ${{ matrix.board }}
```

### 9.3 Regression Testing

#### 9.3.1 Golden Reference Comparison

Ensure refactored code produces identical results:

```python
# tests/regression_test.py
def test_fft_output_unchanged():
    # Load pre-refactoring FFT output (golden reference)
    golden = np.load('tests/golden/fft_256x256_output.npy')

    # Run post-refactoring code
    result = run_embedDIP_fft('tests/inputs/test_image_256x256.raw')

    # Compare
    np.testing.assert_allclose(result, golden, rtol=1e-5, atol=1e-8)
```

### 9.4 Performance Testing

Verify no performance regression:

```c
// tests/benchmark.c
void benchmark_fft(void) {
    Image* img = createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE);
    Image* fft_result = createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE);

    uint32_t start = HAL_GetTick();

    for (int i = 0; i < 100; i++) {
        fft(img, fft_result);
    }

    uint32_t end = HAL_GetTick();
    uint32_t avg_ms = (end - start) / 100;

    printf("Average FFT time: %u ms\n", avg_ms);

    // Verify performance hasn't regressed
    assert(avg_ms < BASELINE_FFT_TIME_MS * 1.05);  // Allow 5% margin
}
```

---

## 10. Glossary

### Architecture
The CPU instruction set and features (ARM Cortex-M7, Xtensa LX6). Code that uses CPU-specific instructions or optimized libraries (CMSIS-DSP, ESP-DSP).

### Board / BSP (Board Support Package)
Specific hardware platform with defined peripherals, pins, and memory layout (STM32F746G-Discovery, ESP32-CAM). Includes pin mappings, memory addresses, initialization sequences.

### Portable Code
Generic algorithms that work on any platform without modification. Uses only standard C/C++ library, no hardware dependencies.

### HAL (Hardware Abstraction Layer)
Interface that hides hardware differences. In this document, refers to the Architecture Layer that abstracts CPU-specific features.

### CMSIS-DSP
ARM Cortex Microcontroller Software Interface Standard - Digital Signal Processing. Optimized math library for ARM Cortex-M processors.

### ESP-DSP
Espressif Digital Signal Processing library. Optimized for Xtensa processors used in ESP32.

### DCMI (Digital Camera Interface)
STM32 peripheral for connecting camera sensors directly to the MCU.

### LTDC (LCD-TFT Display Controller)
STM32 peripheral for driving LCD displays with RGB interface.

### PSRAM (Pseudo-Static RAM)
External RAM used in ESP32 modules, accessed via SPI.

### FPU (Floating-Point Unit)
Hardware accelerator for floating-point math operations.

### ISA (Instruction Set Architecture)
The set of instructions a CPU can execute (ARM, Xtensa, RISC-V, x86).

---

## 11. Conclusion

This refactoring separates architecture-specific code (CPU optimizations) from board-specific code (hardware configuration), enabling:

1. **Code Reuse**: Share ARM Cortex-M7 FFT across multiple STM32 boards
2. **Scalability**: Add new boards with minimal code (~100 lines)
3. **Maintainability**: Bug fixes apply to all boards automatically
4. **Clarity**: Clear separation makes codebase easier to understand

The three-layer model (Portable → Architecture → Board) provides a clean foundation for supporting diverse hardware platforms while maintaining a single, unified codebase.

**Next Steps:**
1. Review and approve this design
2. Begin Phase 1 implementation (directory setup)
3. Incremental migration following 7-week plan
4. Continuous testing and validation

---

**Document Revision History:**
- v1.0 - March 2026 - Initial design document

**Related Documents:**
- `ARCHITECTURE_REFACTORING.tex` - LaTeX version of this document
- `PORTING_GUIDE.md` - Guide for adding new boards
- `API_REFERENCE.md` - Architecture layer API reference
