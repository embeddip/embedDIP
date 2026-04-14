// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef DISPLAY_H
#define DISPLAY_H

#include "core/image.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Standard RGB565 color codes.
 * Format: RRRRRGGGGGGBBBBB
 */
typedef enum {
    DISPLAY_COLOR_BLACK = 0x0000,       // R=0,   G=0,   B=0
    DISPLAY_COLOR_WHITE = 0xFFFF,       // R=31,  G=63,  B=31
    DISPLAY_COLOR_RED = 0xF800,         // R=31,  G=0,   B=0
    DISPLAY_COLOR_GREEN = 0x07E0,       // R=0,   G=63,  B=0
    DISPLAY_COLOR_BLUE = 0x001F,        // R=0,   G=0,   B=31
    DISPLAY_COLOR_YELLOW = 0xFFE0,      // R=31,  G=63,  B=0
    DISPLAY_COLOR_CYAN = 0x07FF,        // R=0,   G=63,  B=31
    DISPLAY_COLOR_MAGENTA = 0xF81F,     // R=31,  G=0,   B=31
    DISPLAY_COLOR_GRAY = 0x8410,        // R=16,  G=32,  B=16
    DISPLAY_COLOR_LIGHT_GRAY = 0xC618,  // R=25,  G=50,  B=25
    DISPLAY_COLOR_DARK_GRAY = 0x4208,   // R=8,   G=16,  B=8
    DISPLAY_COLOR_ORANGE = 0xFD20,      // R=31,  G=41,  B=0
    DISPLAY_COLOR_PINK = 0xF81F,        // R=31,  G=0,   B=31 (same as magenta)
    DISPLAY_COLOR_BROWN = 0xA145,       // R=20,  G=10,  B=5
    DISPLAY_COLOR_PURPLE = 0x780F,      // R=15,  G=0,   B=15
    DISPLAY_COLOR_NAVY = 0x000F,        // R=0,   G=0,   B=15
    DISPLAY_COLOR_TEAL = 0x0410,        // R=0,   G=32,  B=16
    DISPLAY_COLOR_LIME = 0x07E0,        // R=0,   G=63,  B=0
    DISPLAY_COLOR_MAROON = 0x8000,      // R=16,  G=0,   B=0
    DISPLAY_COLOR_OLIVE = 0x8400,       // R=16,  G=32,  B=0
} displayColor;

/**
 * @brief Standard color formats for display pixel buffers.
 */
typedef enum {
    DISPLAY_COLOR_FORMAT_RGB565 = 0,
    DISPLAY_COLOR_FORMAT_RGB888,
    DISPLAY_COLOR_FORMAT_ARGB8888,
    DISPLAY_COLOR_FORMAT_GRAYSCALE,
    DISPLAY_COLOR_FORMAT_YUV422
} display_color_format_t;

/**
 * @brief Screen orientation settings.
 */
typedef enum {
    DISPLAY_ORIENTATION_0 = 0,  ///< Portrait
    DISPLAY_ORIENTATION_90,     ///< Landscape
    DISPLAY_ORIENTATION_180,    ///< Inverted Portrait
    DISPLAY_ORIENTATION_270     ///< Inverted Landscape
} display_orientation_t;

/**
 * @brief Result status codes for display functions.
 */
typedef enum {
    DISPLAY_OK = 0,
    DISPLAY_ERROR_GENERIC = -1,
    DISPLAY_ERROR_NOT_SUPPORTED = -2,
    DISPLAY_ERROR_INVALID_ARG = -3,
    DISPLAY_ERROR_BUSY = -4,
    DISPLAY_ERROR_TIMEOUT = -5,
    DISPLAY_ERROR_NO_MEMORY = -6
} display_status_t;

/**
 * @brief Power state of the display.
 */
typedef enum { DISPLAY_POWER_OFF = 0, DISPLAY_POWER_ON } display_power_state_t;

/**
 * @brief Display backlight control mode (optional).
 */
typedef enum {
    DISPLAY_BRIGHTNESS_OFF = 0,
    DISPLAY_BRIGHTNESS_LOW = 25,
    DISPLAY_BRIGHTNESS_MEDIUM = 50,
    DISPLAY_BRIGHTNESS_HIGH = 75,
    DISPLAY_BRIGHTNESS_MAX = 100
} display_brightness_level_t;

/**
 * @brief Standard display information structure.
 */
typedef struct {
    uint16_t width;
    uint16_t height;
    display_color_format_t color_format;
} display_info_t;

typedef struct display_interface {
    int (*init)(void);
    int (*deinit)(void);
    int (*reset)(void);
    int (*clear)(displayColor color);
    int (*show)(Image *image);

} display_t;

// External declaration of STM32 implementation
extern display_t stm32_ota5180a;

#ifdef __cplusplus
}
#endif

#endif  // DISPLAY_H
