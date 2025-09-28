/* ========================================================================== */
/*  File: embedDIP_configs.h                                                  */
/*  Brief: User configuration for the EmbedDIP library                        */
/*  SPDX-License-Identifier: BSD-3-Clause                                     */
/* ========================================================================== */
#ifndef EMBED_DIP_CONFIGS_H
#define EMBED_DIP_CONFIGS_H
#pragma once

/**
 * @file embedDIP_configs.h
 * @brief User-editable build configuration for EmbedDIP.
 *
 * Define exactly **one** target board and (optionally) override feature flags
 * and device selections. You may also pass any of these macros via your build
 * system (e.g., `-DTARGET_BOARD_STM32F7=1`); this header preserves
 * externally-provided definitions.
 */

/* -------------------------------------------------------------------------- */
/* Target selection                                                            */
/* -------------------------------------------------------------------------- */
/**
 * @defgroup embedDIP_cfg_target Target selection
 * @brief Choose exactly one target platform.
 * @{
 *
 * @code
 * // Example (CMake):
 * add_compile_definitions(TARGET_BOARD_STM32F7=1)
 * @endcode
 */

/* Uncomment **one** of the following, or define via compiler flags. */
#define TARGET_BOARD_STM32F7 1
/* #define TARGET_BOARD_ESP32   1 */
/* #define TARGET_BOARD_OTHER   1 */

/* Sanity check: ensure exactly one target is selected. */
#if ((defined(TARGET_BOARD_STM32F7) ? 1 : 0) + \
     (defined(TARGET_BOARD_ESP32) ? 1 : 0) +   \
     (defined(TARGET_BOARD_OTHER) ? 1 : 0)) == 0
#error "No target selected: define exactly one of TARGET_BOARD_STM32F7, TARGET_BOARD_ESP32, TARGET_BOARD_OTHER."
#elif ((defined(TARGET_BOARD_STM32F7) ? 1 : 0) + \
       (defined(TARGET_BOARD_ESP32) ? 1 : 0) +   \
       (defined(TARGET_BOARD_OTHER) ? 1 : 0)) > 1
#error "Multiple targets selected: define **only one** of TARGET_BOARD_STM32F7, TARGET_BOARD_ESP32, TARGET_BOARD_OTHER."
#endif
/** @} */ /* end of embedDIP_cfg_target */

/* -------------------------------------------------------------------------- */
/* Board-specific symbols & feature flags                                      */
/* -------------------------------------------------------------------------- */
/**
 * @defgroup embedDIP_cfg_features Feature flags
 * @brief Enable/disable optional subsystems (overridable).
 * @{
 *
 * Each flag defaults to 1 (enabled) when applicable for the target. Define
 * as 0 to disable at compile time.
 *
 * - `ENABLE_UART_LOGGING` : UART-based logging helpers
 * - `ENABLE_IMAGE_PROCESSING` : image processing modules
 * - `ENABLE_CAMERA_INPUT` : camera capture interfaces
 * - `ENABLE_DISPLAY_OUTPUT` : display output interfaces
 */

/* ============================== STM32F7 =================================== */
#if defined(TARGET_BOARD_STM32F7)
/** @brief Vendor-family define for STM32F7. */
#ifndef STM32F7xx
#define STM32F7xx 1
#endif

#ifndef ENABLE_UART_LOGGING
#define ENABLE_UART_LOGGING 1
#endif
#ifndef ENABLE_IMAGE_PROCESSING
#define ENABLE_IMAGE_PROCESSING 1
#endif
#ifndef ENABLE_CAMERA_INPUT
#define ENABLE_CAMERA_INPUT 1
#endif
#ifndef ENABLE_DISPLAY_OUTPUT
#define ENABLE_DISPLAY_OUTPUT 1
#endif

/* Devices available on STM32F7 builds (overridable) */
#ifndef DEVICE_OV5640
#define DEVICE_OV5640 1 /**< OV5640 camera module present. */
#endif
#ifndef DEVICE_RK043FN48H
#define DEVICE_RK043FN48H 1 /**< RK043FN48H display panel present. */
#endif
#ifndef DEVICE_STM32_UART
#define DEVICE_STM32_UART 1 /**< Use STM32 HAL UART backend. */
#endif

/* =============================== ESP32 ==================================== */
#elif defined(TARGET_BOARD_ESP32)
/** @brief Arduino-style arch define for ESP32 builds. */
#ifndef ARDUINO_ARCH_ESP32
#define ARDUINO_ARCH_ESP32 1
#endif

#ifndef ENABLE_UART_LOGGING
#define ENABLE_UART_LOGGING 1
#endif
#ifndef ENABLE_IMAGE_PROCESSING
#define ENABLE_IMAGE_PROCESSING 1
#endif
#ifndef ENABLE_CAMERA_INPUT
#define ENABLE_CAMERA_INPUT 1
#endif
#ifndef ENABLE_DISPLAY_OUTPUT
#define ENABLE_DISPLAY_OUTPUT 0 /* default off unless a display is wired */
#endif

/* Devices available on ESP32 builds (overridable) */
#ifndef DEVICE_OV2640
#define DEVICE_OV2640 1 /**< OV2640 camera module present. */
#endif
#ifndef DEVICE_ESP32_UART
#define DEVICE_ESP32_UART 1 /**< Use ESP32 UART backend. */
#endif

/* ============================== OTHER ===================================== */
#elif defined(TARGET_BOARD_OTHER)
/**
 * @brief Generic/other target: start with minimal defaults and enable what you need.
 * @note Adjust device macros below to match your board.
 */
#ifndef ENABLE_UART_LOGGING
#define ENABLE_UART_LOGGING 0
#endif
#ifndef ENABLE_IMAGE_PROCESSING
#define ENABLE_IMAGE_PROCESSING 1
#endif
#ifndef ENABLE_CAMERA_INPUT
#define ENABLE_CAMERA_INPUT 0
#endif
#ifndef ENABLE_DISPLAY_OUTPUT
#define ENABLE_DISPLAY_OUTPUT 0
#endif

/* Example device toggles (customize for your platform) */
#ifndef DEVICE_OV5640
#define DEVICE_OV5640 0
#endif
#ifndef DEVICE_OV2640
#define DEVICE_OV2640 0
#endif

#else
#error "Unexpected configuration state. This should be unreachable."
#endif
/** @} */ /* end of embedDIP_cfg_features */

/* -------------------------------------------------------------------------- */
/* Derived convenience macros (read-only)                                      */
/* -------------------------------------------------------------------------- */
/**
 * @defgroup embedDIP_cfg_derived Derived macros
 * @brief Non-user-editable convenience macros computed from the config.
 * @{
 */

/* Example: a combined guard you can use for any camera-dependent code paths. */
#if (ENABLE_CAMERA_INPUT)
#define EMBED_DIP_HAS_CAMERA 1
#else
#define EMBED_DIP_HAS_CAMERA 0
#endif

#if (ENABLE_DISPLAY_OUTPUT)
#define EMBED_DIP_HAS_DISPLAY 1
#else
#define EMBED_DIP_HAS_DISPLAY 0
#endif
/** @} */ /* end of embedDIP_cfg_derived */

#endif /* EMBED_DIP_CONFIGS_H */
