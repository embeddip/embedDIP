// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef DEVICE_RK050HR18

    #include "board/stm32h7s/configs.h"
    #include "core/error.h"
    #include "device/display/display.h"

    #include "stm32h7rsxx_hal.h"

// LTDC handle owned/initialized by the application (STM32CubeMX).
extern LTDC_HandleTypeDef hltdc;

    #define LCD_WIDTH 800
    #define LCD_HEIGHT 480
    #define LCD_FRAMEBUFFER ((uint32_t *)(uintptr_t)FRAME_BUFFER)

static int display_init(void)
{
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)(uintptr_t)LCD_FRAMEBUFFER, LTDC_LAYER_1);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);
    return EMBEDDIP_OK;
}

static int display_deinit(void)
{
    HAL_LTDC_DeInit(&hltdc);
    return EMBEDDIP_OK;
}

static int display_reset(void)
{
    return EMBEDDIP_OK;
}

static int display_clear(displayColor color)
{
    for (uint32_t i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++) {
        LCD_FRAMEBUFFER[i] = color;
    }
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)(uintptr_t)LCD_FRAMEBUFFER, LTDC_LAYER_1);
    return EMBEDDIP_OK;
}

static int display_show(Image *inImg)
{
    if (!inImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    switch (inImg->format) {
    case IMAGE_FORMAT_RGB888:
        HAL_LTDC_SetPixelFormat(&hltdc, LTDC_PIXEL_FORMAT_RGB888, LTDC_LAYER_1);
        break;
    case IMAGE_FORMAT_RGB565:
        HAL_LTDC_SetPixelFormat(&hltdc, LTDC_PIXEL_FORMAT_RGB565, LTDC_LAYER_1);
        break;
    case IMAGE_FORMAT_GRAYSCALE:
        HAL_LTDC_SetPixelFormat(&hltdc, LTDC_PIXEL_FORMAT_L8, LTDC_LAYER_1);
        break;
    default:
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    HAL_LTDC_SetWindowSize(&hltdc, inImg->width, inImg->height, LTDC_LAYER_1);
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)(uintptr_t)inImg->pixels, LTDC_LAYER_1);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);
    return EMBEDDIP_OK;
}

display_t stm32h7s_rk050hr18 = {
    .init = display_init,
    .deinit = display_deinit,
    .reset = display_reset,
    .clear = display_clear,
    .show = display_show,
};

#endif
