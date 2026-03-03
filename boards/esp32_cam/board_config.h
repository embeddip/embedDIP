/**
 * @file board_config.h
 * @brief ESP32-CAM board configuration
 *
 * Board: ESP32-CAM (AI-Thinker)
 * Vendor: AI-Thinker
 * URL: https://github.com/raphaelbs/esp32-cam-ai-thinker
 *
 * Key Hardware:
 * - CPU: ESP32 (Dual-core Xtensa LX6 @ 240 MHz)
 * - Flash: 4MB
 * - PSRAM: 4MB (external SPI RAM)
 * - Camera: OV2640 via I2C + parallel interface
 * - MicroSD: Yes (shares SPI bus)
 * - Wi-Fi: 802.11 b/g/n
 * - Bluetooth: Classic + BLE
 * - Programming: Via external USB-Serial adapter
 *
 * Pin Mappings:
 * - Camera data: GPIO 4, 5, 18, 19, 36, 39, 34, 35
 * - Camera control: GPIO 25 (SDA), GPIO 23 (SCL), GPIO 32 (PWDN), GPIO 0 (RESET)
 * - LED flash: GPIO 4 (white LED)
 * - Serial: GPIO 1 (TX), GPIO 3 (RX)
 * - Boot button: GPIO 0
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================================
// Architecture Selection (MANDATORY)
// ============================================================================
// This board uses Xtensa LX6, so we inherit all architecture capabilities
#define ARCH_XTENSA_LX6 1
#include "arch/xtensa_lx6/arch_config.h"

// ============================================================================
// Board Identification
// ============================================================================
#define BOARD_NAME "ESP32-CAM"
#define BOARD_VENDOR "AI-Thinker"
#define BOARD_FAMILY "ESP32"
#define BOARD_MCU "ESP32-D0WDQ6"

// Board-specific define for backward compatibility
#define BOARD_ESP32_CAM 1

// ============================================================================
// Memory Configuration
// ============================================================================

// --- Flash Memory ---
#define BOARD_FLASH_SIZE (4 * 1024 * 1024)  // 4MB

// --- PSRAM (External SPI RAM) ---
#define BOARD_HAS_PSRAM 1
#define BOARD_PSRAM_SIZE (4 * 1024 * 1024)  // 4MB

// ESP32 memory allocation is handled by ESP-IDF heap allocator
// We use PSRAM for large image buffers automatically
// No explicit framebuffer address needed (dynamic allocation)

// --- Memory Pool ---
// ESP32 uses heap_caps_malloc() - no fixed pool needed
// But we define these for compatibility with arch layer
#define BOARD_MEMORY_POOL_BASE 0  // Not used on ESP32
#define BOARD_MEMORY_POOL_SIZE BOARD_PSRAM_SIZE

// ============================================================================
// Camera Configuration (OV2640)
// ============================================================================
#define BOARD_HAS_CAMERA 1
#define BOARD_CAMERA_TYPE "OV2640"
#define BOARD_CAMERA_INTERFACE "I2C + Parallel"

// Camera I2C pins (SCCB protocol, compatible with I2C)
#define BOARD_CAMERA_I2C_SDA 26    // GPIO26: I2C SDA (SCCB_SDA)
#define BOARD_CAMERA_I2C_SCL 27    // GPIO27: I2C SCL (SCCB_SCL)
#define BOARD_CAMERA_I2C_FREQ 100000  // 100kHz

// Camera control pins
#define BOARD_CAMERA_PWDN 32       // GPIO32: Power down (active high)
#define BOARD_CAMERA_RESET 0       // GPIO0: Reset (active low) - shared with boot button!
#define BOARD_CAMERA_XCLK 0        // GPIO0: External clock

// Camera data pins (parallel interface, 8-bit)
#define BOARD_CAMERA_D0 5          // GPIO5
#define BOARD_CAMERA_D1 18         // GPIO18
#define BOARD_CAMERA_D2 19         // GPIO19
#define BOARD_CAMERA_D3 21         // GPIO21
#define BOARD_CAMERA_D4 36         // GPIO36
#define BOARD_CAMERA_D5 39         // GPIO39
#define BOARD_CAMERA_D6 34         // GPIO34
#define BOARD_CAMERA_D7 35         // GPIO35

// Camera sync pins
#define BOARD_CAMERA_VSYNC 25      // GPIO25: Vertical sync
#define BOARD_CAMERA_HREF 23       // GPIO23: Horizontal reference
#define BOARD_CAMERA_PCLK 22       // GPIO22: Pixel clock

// Camera resolutions supported by OV2640
#define BOARD_CAMERA_MAX_WIDTH 1600
#define BOARD_CAMERA_MAX_HEIGHT 1200
#define BOARD_CAMERA_DEFAULT_WIDTH 320
#define BOARD_CAMERA_DEFAULT_HEIGHT 240

// ============================================================================
// LED Configuration
// ============================================================================
#define BOARD_HAS_LED 1
#define BOARD_LED_FLASH_PIN 4      // GPIO4: White LED (high-brightness flash)
#define BOARD_LED_ONBOARD_PIN -1   // No separate onboard LED

// ============================================================================
// MicroSD Card Configuration
// ============================================================================
#define BOARD_HAS_MICROSD 1
#define BOARD_SD_CMD 15            // GPIO15: SD CMD
#define BOARD_SD_CLK 14            // GPIO14: SD CLK
#define BOARD_SD_DATA0 2           // GPIO2: SD DATA0
// Note: SD card shares pins with some camera data lines

// ============================================================================
// Serial Configuration
// ============================================================================
#define BOARD_HAS_UART 1
#define BOARD_UART_TX 1            // GPIO1: Serial TX
#define BOARD_UART_RX 3            // GPIO3: Serial RX
#define BOARD_UART_DEFAULT_BAUDRATE 115200

// Programming serial (requires external USB-Serial adapter)
#define BOARD_UART_PROG_TX 1       // Same as UART_TX
#define BOARD_UART_PROG_RX 3       // Same as UART_RX

// ============================================================================
// Button Configuration
// ============================================================================
#define BOARD_HAS_BUTTON 1
#define BOARD_BUTTON_BOOT 0        // GPIO0: Boot button (also camera reset!)

// ============================================================================
// Clock Configuration
// ============================================================================
#define BOARD_CPU_CLOCK_HZ 240000000    // 240 MHz (can be reduced to save power)
#define BOARD_XTAL_FREQ_HZ 40000000     // 40 MHz external crystal

// ============================================================================
// Wi-Fi / Bluetooth Configuration
// ============================================================================
#define BOARD_HAS_WIFI 1
#define BOARD_HAS_BLUETOOTH 1
#define BOARD_WIFI_ANTENNA_TYPE "PCB"   // PCB trace antenna

// ============================================================================
// Board Capabilities Summary
// ============================================================================
#define BOARD_HAS_EXTERNAL_PSRAM 1
#define BOARD_HAS_LCD 0                 // No built-in LCD
#define BOARD_HAS_ETHERNET 0            // No Ethernet
#define BOARD_HAS_USB 0                 // No USB (requires external adapter)

// ============================================================================
// ESP32-Specific Defines
// ============================================================================
#define ARDUINO_ARCH_ESP32 1
#define ESP32 1

// ============================================================================
// Power Management
// ============================================================================
#define BOARD_SUPPORTS_DEEP_SLEEP 1
#define BOARD_SUPPORTS_LIGHT_SLEEP 1

// Camera power consumption: ~150mA active, <1mA deep sleep
#define BOARD_CAMERA_POWER_ACTIVE_MA 150
#define BOARD_CAMERA_POWER_SLEEP_MA 1

// ============================================================================
// Important Notes
// ============================================================================
/*
 * WARNING: GPIO0 is shared between camera reset and boot button!
 * - During programming: GPIO0 must be LOW (boot mode)
 * - During normal operation: GPIO0 controls camera reset
 * - This can cause conflicts - handle with care
 *
 * PSRAM: Required for image processing with embedDIP
 * - Verify PSRAM is detected: psramFound() should return true
 * - Without PSRAM, only small images can be processed
 *
 * Power Supply: ESP32-CAM requires stable 5V @ 500mA+
 * - Insufficient power causes brownouts and resets
 * - Use quality power supply, not weak USB ports
 */

// ============================================================================
// Board-Specific Validation
// ============================================================================
#if !defined(ARCH_XTENSA_LX6)
    #error "This board requires Xtensa LX6 architecture"
#endif

#endif // BOARD_CONFIG_H
