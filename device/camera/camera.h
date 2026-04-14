// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef CAMERA_H
#define CAMERA_H

#include "core/image.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { CONTINUOUS = 0, SINGLE = 1 } captureMode;

#define CAMERA_R160x120 IMAGE_RES_QQVGA /* QQVGA Resolution                     */
#define CAMERA_R320x240 IMAGE_RES_QVGA  /* QVGA Resolution                      */
#define CAMERA_R480x272 IMAGE_RES_WQVGA /* 480x272 Resolution                   */
#define CAMERA_R640x480 IMAGE_RES_VGA   /* VGA Resolution                       */

#define CAMERA_CONTRAST_BRIGHTNESS 0x00 /* Camera contrast brightness features  */
#define CAMERA_BLACK_WHITE 0x01         /* Camera black white feature           */
#define CAMERA_COLOR_EFFECT 0x03        /* Camera color effect feature          */

#define CAMERA_BRIGHTNESS_LEVEL0 0x00 /* Brightness level -2         */
#define CAMERA_BRIGHTNESS_LEVEL1 0x01 /* Brightness level -1         */
#define CAMERA_BRIGHTNESS_LEVEL2 0x02 /* Brightness level 0          */
#define CAMERA_BRIGHTNESS_LEVEL3 0x03 /* Brightness level +1         */
#define CAMERA_BRIGHTNESS_LEVEL4 0x04 /* Brightness level +2         */

#define CAMERA_CONTRAST_LEVEL0 0x05 /* Contrast level -2           */
#define CAMERA_CONTRAST_LEVEL1 0x06 /* Contrast level -1           */
#define CAMERA_CONTRAST_LEVEL2 0x07 /* Contrast level  0           */
#define CAMERA_CONTRAST_LEVEL3 0x08 /* Contrast level +1           */
#define CAMERA_CONTRAST_LEVEL4 0x09 /* Contrast level +2           */

#define CAMERA_BLACK_WHITE_BW 0x00          /* Black and white effect      */
#define CAMERA_BLACK_WHITE_NEGATIVE 0x01    /* Negative effect             */
#define CAMERA_BLACK_WHITE_BW_NEGATIVE 0x02 /* BW and Negative effect      */
#define CAMERA_BLACK_WHITE_NORMAL 0x03      /* Normal effect               */

#define CAMERA_COLOR_EFFECT_NONE 0x00    /* No effects                  */
#define CAMERA_COLOR_EFFECT_BLUE 0x01    /* Blue effect                 */
#define CAMERA_COLOR_EFFECT_GREEN 0x02   /* Green effect                */
#define CAMERA_COLOR_EFFECT_RED 0x03     /* Red effect                  */
#define CAMERA_COLOR_EFFECT_ANTIQUE 0x04 /* Antique effect              */

typedef struct camera_interface {
    int (*init)(ImageResolution resolution, ImageFormat format);
    int (*capture)(captureMode mode, Image *inImg);
    int (*stop)(void);
    int (*setRes)(ImageResolution resolution);
} camera_t;

// External declaration of STM32 implementation
extern camera_t stm32_ov5640;
extern camera_t esp32_ov2640;

// Frame capture completion flag (set by DCMI interrupt)
extern volatile uint8_t frame_capture_complete;

typedef struct {
    void (*Init)(uint16_t, uint32_t);
    uint16_t (*ReadID)(uint16_t);
    void (*Config)(uint16_t, uint32_t, uint32_t, uint32_t);
} CAMERA_DrvTypeDef;

#ifdef __cplusplus
}
#endif

#endif  // CAMERA_H
