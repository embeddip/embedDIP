/* ========================================================================== */
/*  File: camera.h                                                            */
/*  Brief: Generic camera interface for EmbedDIP                              */
/*  SPDX-License-Identifier: MIT                                              */
/* ========================================================================== */
#ifndef CAMERA_H
#define CAMERA_H
#pragma once

/**
 * @file camera.h
 * @brief Platform-agnostic camera interface and controls for the EmbedDIP library.
 *
 * This header defines common camera resolutions, effects, control macros, and a
 * minimal function-pointer-based interface (::camera_t) to integrate platform-
 * specific drivers (e.g., STM32 OV5640, ESP32 OV2640).
 */

#include <stdint.h>
#include <stddef.h>
#include "core/image.h" /* ::Image, ::ImageResolution */

#ifdef __cplusplus
extern "C"
{
#endif

    /* -------------------------------------------------------------------------- */
    /* Capture mode                                                               */
    /* -------------------------------------------------------------------------- */
    /**
     * @enum captureMode
     * @brief Capture mode selection.
     */
    typedef enum
    {
        CONTINUOUS = 0, /**< Continuous streaming (free-run). */
        SINGLE = 1      /**< One-shot capture (single frame). */
    } captureMode;

/* -------------------------------------------------------------------------- */
/* Resolution aliases (map to ::ImageResolution)                              */
/* -------------------------------------------------------------------------- */
/** @name Camera resolution aliases
 *  @brief Convenience macros mapping to ::ImageResolution values.
 *  @{
 */
#define CAMERA_R160x120 IMAGE_RES_QQVGA /**< QQVGA 160×120 */
#define CAMERA_R320x240 IMAGE_RES_QVGA  /**< QVGA  320×240 */
#define CAMERA_R480x272 IMAGE_RES_WQVGA /**< WQVGA 480×272 */
#define CAMERA_R640x480 IMAGE_RES_VGA   /**< VGA   640×480 */
/** @} */

/* -------------------------------------------------------------------------- */
/* Control categories                                                         */
/* -------------------------------------------------------------------------- */
#define CAMERA_CONTRAST_BRIGHTNESS 0x00 /**< Contrast/Brightness control group. */
#define CAMERA_BLACK_WHITE 0x01         /**< Black & White / Negative modes.   */
#define CAMERA_COLOR_EFFECT 0x03        /**< Color effects group.              */

/* -------------------------------------------------------------------------- */
/* Brightness levels                                                          */
/* -------------------------------------------------------------------------- */
/** @name Brightness levels
 *  @brief Discrete brightness settings (device-dependent mapping).
 *  @{
 */
#define CAMERA_BRIGHTNESS_LEVEL0 0x00 /**< Brightness -2 */
#define CAMERA_BRIGHTNESS_LEVEL1 0x01 /**< Brightness -1 */
#define CAMERA_BRIGHTNESS_LEVEL2 0x02 /**< Brightness  0 */
#define CAMERA_BRIGHTNESS_LEVEL3 0x03 /**< Brightness +1 */
#define CAMERA_BRIGHTNESS_LEVEL4 0x04 /**< Brightness +2 */
/** @} */

/* -------------------------------------------------------------------------- */
/* Contrast levels                                                            */
/* -------------------------------------------------------------------------- */
/** @name Contrast levels
 *  @brief Discrete contrast settings (device-dependent mapping).
 *  @{
 */
#define CAMERA_CONTRAST_LEVEL0 0x05 /**< Contrast -2 */
#define CAMERA_CONTRAST_LEVEL1 0x06 /**< Contrast -1 */
#define CAMERA_CONTRAST_LEVEL2 0x07 /**< Contrast  0 */
#define CAMERA_CONTRAST_LEVEL3 0x08 /**< Contrast +1 */
#define CAMERA_CONTRAST_LEVEL4 0x09 /**< Contrast +2 */
/** @} */

/* -------------------------------------------------------------------------- */
/* Black & White / Negative modes                                             */
/* -------------------------------------------------------------------------- */
/** @name Black & White / Negative
 *  @brief Monochrome/negative presets (device-dependent mapping).
 *  @{
 */
#define CAMERA_BLACK_WHITE_BW 0x00          /**< Black & white.         */
#define CAMERA_BLACK_WHITE_NEGATIVE 0x01    /**< Negative.               */
#define CAMERA_BLACK_WHITE_BW_NEGATIVE 0x02 /**< BW + Negative combined. */
#define CAMERA_BLACK_WHITE_NORMAL 0x03      /**< Normal (color).         */
/** @} */

/* -------------------------------------------------------------------------- */
/* Color effects                                                              */
/* -------------------------------------------------------------------------- */
/** @name Color effects
 *  @brief Color tint presets (device-dependent mapping).
 *  @{
 */
#define CAMERA_COLOR_EFFECT_NONE 0x00    /**< No effect.   */
#define CAMERA_COLOR_EFFECT_BLUE 0x01    /**< Blue tint.   */
#define CAMERA_COLOR_EFFECT_GREEN 0x02   /**< Green tint.  */
#define CAMERA_COLOR_EFFECT_RED 0x03     /**< Red tint.    */
#define CAMERA_COLOR_EFFECT_ANTIQUE 0x04 /**< Antique/sepia*/
    /** @} */

    /* -------------------------------------------------------------------------- */
    /* Camera interface                                                           */
    /* -------------------------------------------------------------------------- */
    /**
     * @struct camera_interface
     * @brief Function-pointer interface for a camera implementation.
     *
     * Drivers should populate an instance of this struct with platform-specific
     * implementations. Functions return 0 on success, non-zero on failure.
     */
    typedef struct camera_interface
    {
        /**
         * @brief Initialize the camera with a given resolution.
         * @param resolution Target resolution (see ::ImageResolution).
         * @retval 0 on success
         * @retval non-zero on failure (device not found, unsupported mode, etc.)
         */
        int (*init)(ImageResolution resolution);

        /**
         * @brief Capture a frame.
         * @param mode   ::SINGLE for one-shot, ::CONTINUOUS for streaming mode.
         * @param inImg  Output image container. Width/height/format should be set
         *               or will be filled by the driver depending on implementation.
         * @retval 0 on success
         * @retval non-zero on failure
         *
         * @note For streaming (CONTINUOUS), some drivers may require prior DMA/ISR
         *       setup and return immediately while filling buffers asynchronously.
         */
        int (*capture)(captureMode mode, Image *inImg);

        /**
         * @brief Stop an ongoing capture/stream.
         * @retval 0 on success
         * @retval non-zero on failure
         */
        int (*stop)(void);

        /**
         * @brief Change the active resolution.
         * @param resolution New target resolution.
         * @retval 0 on success
         * @retval non-zero on failure
         *
         * @note Some devices may require a full re-init or power cycle to apply.
         */
        int (*setRes)(ImageResolution resolution);
    } camera_t;

    /* -------------------------------------------------------------------------- */
    /* Known driver instances                                                     */
    /* -------------------------------------------------------------------------- */
    /**
     * @brief STM32 + OV5640 camera driver instance (provided by platform layer).
     */
    extern camera_t stm32_ov5640;

    /**
     * @brief ESP32 + OV2640 camera driver instance (provided by platform layer).
     */
    extern camera_t esp32_ov2640;

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_H */
