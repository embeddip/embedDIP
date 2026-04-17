// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

/**
 * @file ov5640.h
 * @brief Consolidated OV5640 camera driver header
 *
 * This header provides OV5640 camera driver interfaces for STM32 platforms.
 * All I2C communication and platform-specific code is internal to the implementation.
 */

#include <embedDIP_configs.h>

#ifdef DEVICE_OV5640

    #ifndef __OV5640_H
        #define __OV5640_H

        #ifdef __cplusplus
extern "C" {
        #endif

        #include "device/camera/camera.h"

        /**
         * @brief OV5640 chip ID
         */
        #define OV5640_ID 0x5640

        /**
         * @brief OV5640 I2C address (7-bit address shifted left by 1)
         */
        #define CAMERA_I2C_ADDRESS 0x78

        /**
         * @brief OV5640 pixel format constants
         */
        #define OV5640_PIXEL_FORMAT_Y8 0x10     /**< 8-bit grayscale */
        #define OV5640_PIXEL_FORMAT_RGB565 0x6F /**< RGB565 format */
        #define OV5640_PIXEL_FORMAT_RGB888 0x23 /**< RGB888 format */
        #define OV5640_PIXEL_FORMAT_YUV422 0x30 /**< YUV422 format */

/**
 * @brief Initialize OV5640 camera
 * @param DeviceAddr: I2C device address (typically 0x78)
 * @param resolution: Camera resolution (CAMERA_R160x120, CAMERA_R320x240, CAMERA_R480x272,
 * CAMERA_R640x480)
 */
void ov5640_Init(uint16_t DeviceAddr, uint32_t resolution);

/**
 * @brief Read OV5640 chip ID
 * @param DeviceAddr: I2C device address
 * @retval 16-bit chip ID (should be 0x5640)
 */
uint16_t ov5640_ReadID(uint16_t DeviceAddr);

/**
 * @brief Set OV5640 pixel format
 * @param DeviceAddr: I2C device address
 * @param format: Pixel format (OV5640_PIXEL_FORMAT_Y8, RGB565, RGB888, YUV422)
 * @retval 0 on success, -1 on error
 */
int OV5640_SetPixelFormat(uint16_t DeviceAddr, uint32_t format);

/**
 * @brief OV5640 camera driver structure (legacy interface)
 */
extern CAMERA_DrvTypeDef ov5640_drv;

/**
 * @brief STM32 OV5640 camera interface implementation
 */
extern camera_t stm32_ov5640;

/**
 * @brief Frame capture completion flag (set by DCMI interrupt)
 */
extern volatile uint8_t frame_capture_complete;

        #ifdef __cplusplus
}
        #endif

    #endif /* __OV5640_H */

#endif /* DEVICE_OV5640 */
