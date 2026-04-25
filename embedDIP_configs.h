// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBED_DIP_CONFIGS_H
#define EMBED_DIP_CONFIGS_H
#pragma once

/**
 * @file embedDIP_configs.h
 * @brief User-editable build configuration for EmbedDIP.
 *
 * Select exactly one board, one architecture family, and one CPU variant via
 * compiler defines.
 * Typical usage with CMake: set EMBEDDIP_TARGET_BOARD, EMBEDDIP_ARCH, EMBEDDIP_CPU.
 */

/* -------------------------------------------------------------------------- */
/* Hard-switch guard (legacy macros removed)                                   */
/* -------------------------------------------------------------------------- */
#if defined(TARGET_BOARD_STM32F7) || defined(TARGET_BOARD_ESP32) || defined(TARGET_BOARD_OTHER)
    #error                                                                                         \
        "Legacy TARGET_BOARD_* macros are not supported. Use EMBED_DIP_BOARD_* and EMBED_DIP_ARCH_* instead."
#endif

/* -------------------------------------------------------------------------- */
/* Target selection                                                            */
/* -------------------------------------------------------------------------- */
/* Uncomment only if you do not provide these from the build system. */
/* #define EMBED_DIP_BOARD_STM32F7 1 */
/* #define EMBED_DIP_BOARD_ESP32   1 */

/* #define EMBED_DIP_ARCH_ARM     1 */
/* #define EMBED_DIP_ARCH_XTENSA  1 */

/* #define EMBED_DIP_CPU_CORTEX_M7 1 */
/* #define EMBED_DIP_CPU_LX6       1 */
/* #define EMBED_DIP_CPU_LX7       1 */

/* -------------------------------------------------------------------------- */
/* Arduino auto-detection (Library Manager friendly defaults)                 */
/* -------------------------------------------------------------------------- */
/*
 * If the build system does not provide explicit EMBED_DIP_* target macros,
 * infer them from Arduino core/platform macros so sketches can compile
 * without extra CLI flags.
 */
#if !defined(EMBED_DIP_BOARD_STM32F7) && !defined(EMBED_DIP_BOARD_ESP32)
    #if defined(ARDUINO_ARCH_ESP32)
        #define EMBED_DIP_BOARD_ESP32 1
    #elif defined(STM32F7xx)
        #define EMBED_DIP_BOARD_STM32F7 1
    #endif
#endif

#if !defined(EMBED_DIP_ARCH_ARM) && !defined(EMBED_DIP_ARCH_XTENSA)
    #if defined(EMBED_DIP_BOARD_ESP32)
        #define EMBED_DIP_ARCH_XTENSA 1
    #elif defined(EMBED_DIP_BOARD_STM32F7)
        #define EMBED_DIP_ARCH_ARM 1
    #endif
#endif

#if !defined(EMBED_DIP_CPU_CORTEX_M7) && !defined(EMBED_DIP_CPU_LX6) && !defined(EMBED_DIP_CPU_LX7)
    #if defined(EMBED_DIP_BOARD_ESP32)
        /*
         * ESP32/ESP32-S2/ESP32-S3 families are LX6/LX7. Prefer explicit IDF
         * target hints when available; default to LX6 for classic ESP32.
         */
        #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
            #define EMBED_DIP_CPU_LX7 1
        #else
            #define EMBED_DIP_CPU_LX6 1
        #endif
    #elif defined(EMBED_DIP_BOARD_STM32F7)
        #define EMBED_DIP_CPU_CORTEX_M7 1
    #endif
#endif

/* Sanity check: exactly one board. */
#if ((defined(EMBED_DIP_BOARD_STM32F7) ? 1 : 0) + (defined(EMBED_DIP_BOARD_ESP32) ? 1 : 0)) == 0
    #error                                                                                         \
        "No board selected: define exactly one of EMBED_DIP_BOARD_STM32F7 or EMBED_DIP_BOARD_ESP32."
#elif ((defined(EMBED_DIP_BOARD_STM32F7) ? 1 : 0) + (defined(EMBED_DIP_BOARD_ESP32) ? 1 : 0)) > 1
    #error                                                                                         \
        "Multiple boards selected: define only one of EMBED_DIP_BOARD_STM32F7 or EMBED_DIP_BOARD_ESP32."
#endif

/* Sanity check: exactly one architecture family. */
#if ((defined(EMBED_DIP_ARCH_ARM) ? 1 : 0) + (defined(EMBED_DIP_ARCH_XTENSA) ? 1 : 0)) == 0
    #error                                                                                         \
        "No architecture family selected: define exactly one of EMBED_DIP_ARCH_ARM or EMBED_DIP_ARCH_XTENSA."
#elif ((defined(EMBED_DIP_ARCH_ARM) ? 1 : 0) + (defined(EMBED_DIP_ARCH_XTENSA) ? 1 : 0)) > 1
    #error "Multiple architecture families selected: define only one EMBED_DIP_ARCH_* macro."
#endif

/* Sanity check: exactly one CPU variant. */
#if ((defined(EMBED_DIP_CPU_CORTEX_M7) ? 1 : 0) + (defined(EMBED_DIP_CPU_LX6) ? 1 : 0) +           \
     (defined(EMBED_DIP_CPU_LX7) ? 1 : 0)) == 0
    #error                                                                                         \
        "No CPU selected: define exactly one of EMBED_DIP_CPU_CORTEX_M7, EMBED_DIP_CPU_LX6, EMBED_DIP_CPU_LX7."
#elif ((defined(EMBED_DIP_CPU_CORTEX_M7) ? 1 : 0) + (defined(EMBED_DIP_CPU_LX6) ? 1 : 0) +         \
       (defined(EMBED_DIP_CPU_LX7) ? 1 : 0)) > 1
    #error "Multiple CPUs selected: define only one EMBED_DIP_CPU_* macro."
#endif

/* Board/architecture/CPU compatibility matrix. */
#if defined(EMBED_DIP_BOARD_STM32F7)
    #if !(defined(EMBED_DIP_ARCH_ARM) && defined(EMBED_DIP_CPU_CORTEX_M7))
        #error                                                                                     \
            "Invalid combination: EMBED_DIP_BOARD_STM32F7 requires EMBED_DIP_ARCH_ARM + EMBED_DIP_CPU_CORTEX_M7."
    #endif
#elif defined(EMBED_DIP_BOARD_ESP32)
    #if !(defined(EMBED_DIP_ARCH_XTENSA) &&                                                        \
          (defined(EMBED_DIP_CPU_LX6) || defined(EMBED_DIP_CPU_LX7)))
        #error                                                                                     \
            "Invalid combination: EMBED_DIP_BOARD_ESP32 requires EMBED_DIP_ARCH_XTENSA + (EMBED_DIP_CPU_LX6 or EMBED_DIP_CPU_LX7)."
    #endif
#endif

/**
 * @defgroup embedDIP_cfg_features Feature flags
 * @brief Enable/disable optional subsystems.
 * @{
 */

/* ============================== STM32F7 =================================== */
#if defined(EMBED_DIP_BOARD_STM32F7)
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

    #ifndef DEVICE_OV5640
        #define DEVICE_OV5640 1
    #endif
    #ifndef DEVICE_RK043FN48H
        #define DEVICE_RK043FN48H 1
    #endif
    #ifndef DEVICE_STM32_UART
        #define DEVICE_STM32_UART 1
    #endif

/* =============================== ESP32 ==================================== */
#elif defined(EMBED_DIP_BOARD_ESP32)
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
        #define ENABLE_DISPLAY_OUTPUT 0
    #endif

    #ifndef DEVICE_OV2640
        #define DEVICE_OV2640 1
    #endif
    #ifndef DEVICE_ESP32_UART
        #define DEVICE_ESP32_UART 1
    #endif
#endif

/** @} */ /* end of embedDIP_cfg_features */

/**
 * @defgroup embedDIP_cfg_derived Derived macros
 * @brief Non-user-editable convenience macros computed from the config.
 * @{
 */
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
