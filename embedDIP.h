// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBED_DIP_H
#define EMBED_DIP_H

/**
 * @file embed_dip.h
 * @brief Public umbrella header for the EmbedDIP library.
 *
 * Include this header to access the core C APIs.
 *
 * @note C headers are exposed with C linkage when included from C++.
 */

/** @defgroup embedDIP EmbedDIP
 *  @brief Portable embedded digital image processing library.
 *
 */

#ifdef __cplusplus
extern "C" {
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

/**
 * @defgroup embedDIP_ver Version
 * @brief Semantic version of the library.
 * @{
 */

/** @brief Major version (breaking changes). */
#define EMBED_DIP_VERSION_MAJOR 1U
/** @brief Minor version (new features, backward compatible). */
#define EMBED_DIP_VERSION_MINOR 1U
/** @brief Patch version (bug fixes, no API changes). */
#define EMBED_DIP_VERSION_PATCH 0U

/** @brief Compose a single integer version (MMmmpp). */
#define EMBED_DIP_VERSION_CODE                                                                     \
    ((EMBED_DIP_VERSION_MAJOR * 10000U) + (EMBED_DIP_VERSION_MINOR * 100U) +                       \
     (EMBED_DIP_VERSION_PATCH))

/** @} */ /* end of embedDIP_ver */

/**
 * @defgroup embedDIP_c_api C API
 * @brief C headers exposed with C linkage for use in C and C++.
 * @{
 */

/* Core APIs */
#include "board/common.h"                /**< Board-level helpers and timing. */
#include "core/error.h"                  /**< Error handling and status codes. */
#include "core/image.h"                  /**< Image type and utilities. */
#include "core/memory_manager.h"         /**< Allocators and memory helpers. */
#include "device/serial/serial.h"        /**< Serial I/O abstraction. */
#include "imgproc/color.h"               /**< Color conversions and helpers. */
#include "imgproc/compress.h"            /**< JPEG compression helper. */
#include "imgproc/connectedcomponents.h" /**< Connected components labeling. */
#include "imgproc/drawing.h"             /**< Drawing primitives and shapes. */
#include "imgproc/fft.h"                 /**< Frequency-domain processing. */
#include "imgproc/filter.h"              /**< Spatial filtering and kernels. */
#include "imgproc/histogram.h"           /**< Histogram ops and equalization. */
#include "imgproc/hough.h"               /**< Hough transform for line/circle detection. */
#include "imgproc/imgwarp.h"             /**< Geometric transformations (warp, rotate, scale). */
#include "imgproc/misc.h"                /**< Miscellaneous image utilities. */
#include "imgproc/morph.h"               /**< Morphological operations (erode, dilate). */
#include "imgproc/pixel.h"               /**< Pixel operations (negative, etc). */
#include "imgproc/segmentation.h"        /**< Image segmentation algorithms. */
#include "imgproc/thresh.h"              /**< Thresholding operations. */

/**
 * @defgroup embedDIP_hw
 * @brief Optional device headers enabled via platform defines.
 * @{
 */

#if defined(EMBED_DIP_BOARD_ESP32)
    #include "device/camera/camera.h" /**< Camera abstraction. */
#endif

#if defined(EMBED_DIP_BOARD_STM32F7)
    #include "device/camera/camera.h"   /**< Camera abstraction. */
    #include "device/display/display.h" /**< Display abstraction. */
#endif

/** @} */ /* end of embedDIP_hw */
/** @} */ /* end of embedDIP_c_api */

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
 * @defgroup embedDIP_cpp_api C++ API
 * @note These headers use C++ linkage and must not be wrapped in `extern "C"`.
 * @{
 */
#ifdef __cplusplus
    #include "wrapper/CameraWrapper.hpp"
    #include "wrapper/DisplayWrapper.hpp"
    #include "wrapper/ImageWrapper.hpp"
    #include "wrapper/MemoryManager.hpp"
    #include "wrapper/SerialWrapper.hpp"
#endif
/** @} */ /* end of embedDIP_cpp_api */

/** @} */ /* end of embedDIP */

#endif /* EMBED_DIP_H */
