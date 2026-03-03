/**
 * @file board_config.h
 * @brief STM32F746G-Discovery board configuration
 *
 * Board: STM32F746G-Discovery (STM32F746NGH6)
 * Vendor: STMicroelectronics
 * URL: https://www.st.com/en/evaluation-tools/32f746gdiscovery.html
 *
 * Key Hardware:
 * - CPU: STM32F746NGH6 (ARM Cortex-M7 @ 216 MHz)
 * - Flash: 1MB internal
 * - SRAM: 320KB internal (SRAM1: 240KB, SRAM2: 16KB, ITCM: 16KB, DTCM: 64KB)
 * - SDRAM: 8MB external (IS42S16400J)
 * - Camera: OV5640 via DCMI interface
 * - LCD: 4.3" RK043FN48H-CT672B (480x272, RGB interface via LTDC)
 * - Arduino headers: Yes
 * - Audio: SAI audio codec
 * - Ethernet: 10/100 Mbit/s
 *
 * Memory Map:
 * - 0x08000000-0x080FFFFF: Flash (1MB)
 * - 0x20000000-0x2004FFFF: SRAM1 (320KB)
 * - 0xC0000000-0xC07FFFFF: SDRAM (8MB) - Used for framebuffers and dynamic allocation
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================================
// Architecture Selection (MANDATORY)
// ============================================================================
// This board uses ARM Cortex-M7, so we inherit all architecture capabilities
#define ARCH_ARM_CORTEX_M7 1
#include "arch/arm_cortex_m7/arch_config.h"

// ============================================================================
// Board Identification
// ============================================================================
#define BOARD_NAME "STM32F746G-Discovery"
#define BOARD_VENDOR "STMicroelectronics"
#define BOARD_FAMILY "STM32F7"
#define BOARD_MCU "STM32F746NGH6"

// Board-specific define for backward compatibility
#define BOARD_STM32F746G_DISCOVERY 1

// ============================================================================
// Memory Configuration
// ============================================================================

// --- Flash Memory ---
#define BOARD_FLASH_BASE 0x08000000
#define BOARD_FLASH_SIZE (1024 * 1024)  // 1MB

// --- Internal SRAM ---
#define BOARD_SRAM_BASE 0x20000000
#define BOARD_SRAM_SIZE (320 * 1024)    // 320KB total

// --- External SDRAM (IS42S16400J) ---
#define BOARD_SDRAM_BASE 0xC0000000
#define BOARD_SDRAM_SIZE (8 * 1024 * 1024)  // 8MB

// --- Frame Buffer Allocation (in SDRAM) ---
// Camera and LCD share the same framebuffer for zero-copy operation
#define BOARD_FRAMEBUFFER_BASE BOARD_SDRAM_BASE
#define BOARD_FRAMEBUFFER_SIZE (480 * 272 * 4)  // ARGB8888 format
// = 522,240 bytes (rounded to 512KB for safety)
#define BOARD_FRAMEBUFFER_SIZE_ALIGNED (512 * 1024)

// --- Dynamic Memory Pool (in SDRAM, after framebuffer) ---
#define BOARD_MEMORY_POOL_BASE (BOARD_FRAMEBUFFER_BASE + BOARD_FRAMEBUFFER_SIZE_ALIGNED)
#define BOARD_MEMORY_POOL_SIZE (BOARD_SDRAM_SIZE - BOARD_FRAMEBUFFER_SIZE_ALIGNED)
// = ~7.5MB available for image processing

// ============================================================================
// Camera Configuration (OV5640)
// ============================================================================
#define BOARD_HAS_CAMERA 1
#define BOARD_CAMERA_TYPE "OV5640"
#define BOARD_CAMERA_INTERFACE "DCMI"      // Digital Camera Interface

// Camera control pins
#define BOARD_CAMERA_PWR_PIN GPIO_PIN_13   // PH13: Power control (active high)
#define BOARD_CAMERA_PWR_PORT GPIOH
#define BOARD_CAMERA_RST_PIN GPIO_PIN_14   // PH14: Reset (active low)
#define BOARD_CAMERA_RST_PORT GPIOH

// Camera resolutions supported
#define BOARD_CAMERA_MAX_WIDTH 2592
#define BOARD_CAMERA_MAX_HEIGHT 1944
#define BOARD_CAMERA_DEFAULT_WIDTH 480
#define BOARD_CAMERA_DEFAULT_HEIGHT 272

// ============================================================================
// LCD Configuration (RK043FN48H)
// ============================================================================
#define BOARD_HAS_LCD 1
#define BOARD_LCD_TYPE "RK043FN48H-CT672B"
#define BOARD_LCD_INTERFACE "LTDC"         // LCD-TFT Display Controller
#define BOARD_LCD_WIDTH 480
#define BOARD_LCD_HEIGHT 272
#define BOARD_LCD_BPP 32                   // Bits per pixel (ARGB8888)

// LCD timing parameters (for LTDC configuration)
#define BOARD_LCD_HSYNC 41
#define BOARD_LCD_HBP 13
#define BOARD_LCD_HFP 32
#define BOARD_LCD_VSYNC 10
#define BOARD_LCD_VBP 2
#define BOARD_LCD_VFP 2

// LCD backlight control
#define BOARD_LCD_BL_CTRL_PIN GPIO_PIN_3   // PK3: Backlight control
#define BOARD_LCD_BL_CTRL_PORT GPIOK

// ============================================================================
// UART Configuration
// ============================================================================
#define BOARD_HAS_UART 1
#define BOARD_UART_DEFAULT_PORT 1          // USART1
#define BOARD_UART_DEFAULT_BAUDRATE 115200

// UART1 pins (for serial console)
#define BOARD_UART_TX_PIN GPIO_PIN_9       // PA9: USART1_TX
#define BOARD_UART_TX_PORT GPIOA
#define BOARD_UART_RX_PIN GPIO_PIN_10      // PA10: USART1_RX
#define BOARD_UART_RX_PORT GPIOA

// ============================================================================
// Clock Configuration
// ============================================================================
#define BOARD_CPU_CLOCK_HZ 216000000       // 216 MHz
#define BOARD_APB1_CLOCK_HZ 54000000       // 54 MHz (for timers, I2C, etc.)
#define BOARD_APB2_CLOCK_HZ 108000000      // 108 MHz (for SPI, USART, etc.)
#define BOARD_SDRAM_CLOCK_HZ 108000000     // 108 MHz

// ============================================================================
// Board Capabilities Summary
// ============================================================================
#define BOARD_HAS_EXTERNAL_SDRAM 1
#define BOARD_HAS_ETHERNET 1
#define BOARD_HAS_USB_OTG_FS 1
#define BOARD_HAS_USB_OTG_HS 1
#define BOARD_HAS_AUDIO 1
#define BOARD_HAS_ARDUINO_CONNECTOR 1
#define BOARD_HAS_QSPI 1
#define BOARD_HAS_MICROSD 1

// ============================================================================
// STM32-Specific Defines (for HAL compatibility)
// ============================================================================
#define STM32F746xx                        // Chip family
#define USE_HAL_DRIVER                     // Use STM32 HAL library

// ============================================================================
// Board-Specific Validation
// ============================================================================
#if !defined(ARCH_ARM_CORTEX_M7)
    #error "This board requires ARM Cortex-M7 architecture"
#endif

#if BOARD_MEMORY_POOL_SIZE < (1024 * 1024)
    #warning "Memory pool is less than 1MB - may limit image processing capabilities"
#endif

#endif // BOARD_CONFIG_H
