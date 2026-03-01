/* ========================================================================== */
/*  File: embed_dip.h                                                         */
/*  Brief: Public umbrella header for the EmbedDIP library                    */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */
#ifndef EMBED_DIP_H
#define EMBED_DIP_H

/**
 * @file embed_dip.h
 * @brief Public umbrella header for the EmbedDIP library.
 *
 * Include this header to access the core C APIs (image, color, filtering,
 * histogram, FFT, memory manager, device I/O) and, when compiling as C++,
 * the optional RAII-style C++ wrappers.
 *
 * @note C headers are exposed with C linkage when included from C++.
 *       C++ wrapper headers are never wrapped in `extern "C"`.
 */

/** @defgroup embedDIP EmbedDIP
 *  @brief Portable embedded digital image processing library.
 *
 *  This module groups the public C API and optional C++ wrappers for
 *  image representation, processing kernels, device I/O (camera, display,
 *  serial), and helper utilities.
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/* =============================
 * Build Configuration
 * ============================= */
/**
 * @defgroup embedDIP_cfg Build Configuration
 * @brief Compile-time configuration options for EmbedDIP.
 * @{
 */

/**
 * @brief Project-wide configuration include.
 * @details Define board/platform symbols and feature toggles in this file.
 */
#include "embedDIP_configs.h"

/** @} */ /* end of embedDIP_cfg */

/* =============================
 * Versioning
 * ============================= */
/**
 * @defgroup embedDIP_ver Version
 * @brief Semantic version of the library.
 * @{
 */

/** @brief Major version (breaking changes). */
#define EMBED_DIP_VERSION_MAJOR 0U
/** @brief Minor version (new features, backward compatible). */
#define EMBED_DIP_VERSION_MINOR 1U
/** @brief Patch version (bug fixes, no API changes). */
#define EMBED_DIP_VERSION_PATCH 0U

/** @brief Compose a single integer version (MMmmpp). */
#define EMBED_DIP_VERSION_CODE ((EMBED_DIP_VERSION_MAJOR * 10000U) + \
                                (EMBED_DIP_VERSION_MINOR * 100U) +   \
                                (EMBED_DIP_VERSION_PATCH))

/** @} */ /* end of embedDIP_ver */

/* =============================
 * Feature Flags
 * ============================= */
/**
 * @defgroup embedDIP_feat Feature Flags
 * @brief Optional components that can be enabled at build time.
 * @{
 */

/** @brief Enable UART-based logging helpers (1 = enabled, 0 = disabled). */
#ifndef ENABLE_UART_LOGGING
#define ENABLE_UART_LOGGING 1
#endif

/** @brief Enable image processing modules (1 = enabled, 0 = disabled). */
#ifndef ENABLE_IMAGE_PROCESSING
#define ENABLE_IMAGE_PROCESSING 1
#endif

/** @brief Enable camera input interfaces (1 = enabled, 0 = disabled). */
#ifndef ENABLE_CAMERA_INPUT
#define ENABLE_CAMERA_INPUT 1
#endif

/** @brief Enable display output interfaces (1 = enabled, 0 = disabled). */
#ifndef ENABLE_DISPLAY_OUTPUT
#define ENABLE_DISPLAY_OUTPUT 1
#endif

/** @} */ /* end of embedDIP_feat */

/* =============================
 * Public C API (C linkage)
 * ============================= */
/**
 * @defgroup embedDIP_c_api C API
 * @brief C headers exposed with C linkage for use in C and C++.
 * @{
 */

/* Core APIs */
#include "core/error.h"           /**< Error handling and status codes. */
#include "core/image.h"           /**< Image type and utilities. */
#include "imgproc/filter.h"       /**< Spatial filtering and kernels. */
#include "imgproc/histogram.h"    /**< Histogram ops and equalization. */
#include "core/memory_manager.h"  /**< Allocators and memory helpers. */
#include "imgproc/color.h"        /**< Color conversions and helpers. */
#include "imgproc/pixel.h"        /**< Pixel operations (negative, etc). */
#include "board/common.h"         /**< Board-level helpers and timing. */
#include "device/serial/serial.h" /**< Serial I/O abstraction. */
#include "imgproc/fft.h"          /**< Frequency-domain processing. */

    /* =============================
     * Board / Device-Specific (C)
     * ============================= */
    /**
     * @defgroup embedDIP_hw Board / Device-Specific (C)
     * @brief Optional device headers enabled via platform defines.
     * @{
     */

#ifdef ARDUINO_ARCH_ESP32
/* Add ESP32-specific C headers here when available. */
#endif

#if defined(STM32F7xx) || defined(TARGET_BOARD_HOST)
#include "device/display/display.h" /**< Display abstraction. */
#include "device/camera/camera.h"   /**< Camera abstraction. */
#endif

    /** @} */ /* end of embedDIP_hw */
    /** @} */ /* end of embedDIP_c_api */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* =============================
 * C++ Wrappers (no C linkage)
 * ============================= */
/**
 * @defgroup embedDIP_cpp_api C++ API
 * @brief RAII-style wrappers over the C API (C++ only).
 * @note These headers use C++ linkage and must not be wrapped in `extern "C"`.
 * @{
 */
#ifdef __cplusplus
#include "wrapper/ImageWrapper.hpp"
#include "wrapper/CameraWrapper.hpp"
#include "wrapper/DisplayWrapper.hpp"
#include "wrapper/MemoryManager.hpp"
#include "wrapper/SerialWrapper.hpp"
#endif
/** @} */ /* end of embedDIP_cpp_api */

/** @} */ /* end of embedDIP */

#endif /* EMBED_DIP_H */
