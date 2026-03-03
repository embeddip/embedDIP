/**
 * @file board_config.h
 * @brief HOST board configuration (PC/workstation for testing)
 *
 * Board: HOST (x86/x64 PC or Mac)
 * Purpose: Testing and development on PC
 *
 * This configuration allows embedDIP library to be built and tested
 * on a standard PC without embedded hardware. All device drivers use
 * file-based simulation.
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================================
// Architecture Selection (MANDATORY)
// ============================================================================
#define ARCH_HOST 1
#include "arch/host/arch_config.h"

// ============================================================================
// Board Identification
// ============================================================================
#define BOARD_NAME "HOST"
#define BOARD_VENDOR "Generic"
#define BOARD_FAMILY "x86/x64"
#define BOARD_MCU "Native"

// Board-specific define for backward compatibility
#define BOARD_HOST 1

// ============================================================================
// Memory Configuration
// ============================================================================

// HOST uses system RAM - these are just for compatibility
#define BOARD_MEMORY_POOL_BASE 0
#define BOARD_MEMORY_POOL_SIZE 0  // Unlimited (system manages memory)

// ============================================================================
// Simulated Hardware Configuration
// ============================================================================

// Camera (file-based simulation)
#define BOARD_HAS_CAMERA 1
#define BOARD_CAMERA_TYPE "Simulated"
#define BOARD_CAMERA_INTERFACE "File I/O"
#define BOARD_CAMERA_MAX_WIDTH 1920
#define BOARD_CAMERA_MAX_HEIGHT 1080
#define BOARD_CAMERA_DEFAULT_WIDTH 640
#define BOARD_CAMERA_DEFAULT_HEIGHT 480

// Display (file-based output)
#define BOARD_HAS_LCD 1
#define BOARD_LCD_TYPE "Simulated"
#define BOARD_LCD_INTERFACE "File I/O"
#define BOARD_LCD_WIDTH 800
#define BOARD_LCD_HEIGHT 600
#define BOARD_LCD_BPP 32

// Serial (stdin/stdout)
#define BOARD_HAS_UART 1
#define BOARD_UART_DEFAULT_BAUDRATE 115200

// ============================================================================
// Board Capabilities Summary
// ============================================================================
#define BOARD_HAS_EXTERNAL_PSRAM 0
#define BOARD_HAS_EXTERNAL_SDRAM 0
#define BOARD_HAS_ETHERNET 0
#define BOARD_HAS_USB 0
#define BOARD_HAS_WIFI 0
#define BOARD_HAS_BLUETOOTH 0

// ============================================================================
// Important Notes
// ============================================================================
/*
 * HOST Configuration:
 * - All "hardware" is simulated using files
 * - Camera reads from image files (PNG, JPEG, BMP)
 * - Display writes to image files
 * - Serial uses stdin/stdout
 * - Memory uses standard malloc/free
 * - FFT uses software implementation (Cooley-Tukey)
 *
 * Purpose:
 * - Algorithm development and testing without hardware
 * - Unit tests and CI/CD integration
 * - Performance profiling on PC
 * - Cross-platform development
 */

#endif // BOARD_CONFIG_H
