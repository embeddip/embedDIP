// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef DEVICE_OV2640

    #include "device/camera/camera.h"

    #include <string.h>

    #include "Arduino.h"
    #include "driver/gpio.h"
    #include "esp32-hal-ledc.h"
    #include "esp_camera.h"
    #include "esp_log.h"
    #include <esp_system.h>
    #include <nvs_flash.h>
    #include <sys/param.h>

    #define XCLK_GPIO_NUM 4
    #define SIOD_GPIO_NUM 18
    #define SIOC_GPIO_NUM 23
    #define Y9_GPIO_NUM 36
    #define Y8_GPIO_NUM 37
    #define Y7_GPIO_NUM 38
    #define Y6_GPIO_NUM 39
    #define Y5_GPIO_NUM 35
    #define Y4_GPIO_NUM 14
    #define Y3_GPIO_NUM 13
    #define Y2_GPIO_NUM 34
    #define VSYNC_GPIO_NUM 5
    #define HREF_GPIO_NUM 27
    #define PCLK_GPIO_NUM 25
    #define LED_GPIO_NUM 22

static bool g_camera_initialized = false;
static ImageResolution g_current_resolution = IMAGE_RES_QVGA;
static ImageFormat g_current_format = IMAGE_FORMAT_RGB565;

static int camera_deinit_internal(void)
{
    if (!g_camera_initialized) {
        return 0;
    }

    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        return -1;
    }

    // Prevent "GPIO isr service already installed" on subsequent init calls.
    (void)gpio_uninstall_isr_service();

    g_camera_initialized = false;
    return 0;
}

framesize_t map_resolution(ImageResolution resolution)
{
    switch (resolution) {
    case IMAGE_RES_96X96:
        return FRAMESIZE_96X96;
    case IMAGE_RES_QQVGA:
        return FRAMESIZE_QQVGA;
    case IMAGE_RES_QCIF:
        return FRAMESIZE_QCIF;
    case IMAGE_RES_HQVGA:
        return FRAMESIZE_HQVGA;
    case IMAGE_RES_240X240:
        return FRAMESIZE_240X240;
    case IMAGE_RES_QVGA:
        return FRAMESIZE_QVGA;
    case IMAGE_RES_CIF:
        return FRAMESIZE_CIF;
    case IMAGE_RES_HVGA:
        return FRAMESIZE_HVGA;
    case IMAGE_RES_VGA:
        return FRAMESIZE_VGA;
    case IMAGE_RES_SVGA:
        return FRAMESIZE_SVGA;
    case IMAGE_RES_XGA:
        return FRAMESIZE_XGA;
    case IMAGE_RES_HD:
        return FRAMESIZE_HD;
    case IMAGE_RES_SXGA:
        return FRAMESIZE_SXGA;
    case IMAGE_RES_UXGA:
        return FRAMESIZE_UXGA;
    case IMAGE_RES_FHD:
        return FRAMESIZE_FHD;
    case IMAGE_RES_P_HD:
        return FRAMESIZE_P_HD;
    case IMAGE_RES_P_3MP:
        return FRAMESIZE_P_3MP;
    case IMAGE_RES_QXGA:
        return FRAMESIZE_QXGA;
    case IMAGE_RES_QHD:
        return FRAMESIZE_QHD;
    case IMAGE_RES_WQXGA:
        return FRAMESIZE_WQXGA;
    case IMAGE_RES_P_FHD:
        return FRAMESIZE_P_FHD;
    case IMAGE_RES_QSXGA:
        return FRAMESIZE_QSXGA;
    case IMAGE_RES_WQVGA:
        // esp32-camera has no native 480x272. Use HVGA (480x320) and crop in capture.
        return FRAMESIZE_HVGA;
    default:
        return FRAMESIZE_QVGA;
    }
}

int camera_init(ImageResolution resolution, ImageFormat format)
{
    if (g_camera_initialized) {
        if (camera_deinit_internal() != 0) {
            return -1;
        }
    }

    // Keep UART binary protocol clean from ESP-IDF logs.
    esp_log_level_set("camera", ESP_LOG_NONE);
    esp_log_level_set("cam_hal", ESP_LOG_NONE);
    esp_log_level_set("gpio", ESP_LOG_NONE);

    camera_config_t config = {0};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.frame_size = map_resolution(resolution);
    config.pixel_format = (format == IMAGE_FORMAT_GRAYSCALE) ? PIXFORMAT_YUV422 : PIXFORMAT_RGB565;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    #if CONFIG_IDF_TARGET_ESP32
    /* Use PSRAM for larger frame sizes. */
    if (config.pixel_format != PIXFORMAT_JPEG) {
        // Raw paths are more stable at lower XCLK on classic ESP32.
        config.xclk_freq_hz = 10000000;
        if (config.frame_size <= FRAMESIZE_QQVGA) {
            config.fb_location = CAMERA_FB_IN_DRAM;
        } else {
            config.fb_location = CAMERA_FB_IN_PSRAM;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        }
    }
    #endif

    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        return -1;
    }

    sensor_t *s = esp_camera_sensor_get();

    // TODO: We need to explicitly set pixel format on sensor.
    pixformat_t pix_fmt = (format == IMAGE_FORMAT_GRAYSCALE) ? PIXFORMAT_YUV422 : PIXFORMAT_RGB565;
    // if (s->set_pixformat) {
    //     if (s->set_pixformat(s, pix_fmt) != 0) {
    //         return -1;
    //     }
    // }

    if (s->set_framesize) {
        if (s->set_framesize(s, map_resolution(resolution)) != 0) {
            return -1;
        }
    }

    // if (s->set_pixformat) {
    //     if (s->set_pixformat(s, pix_fmt) != 0) {
    //         return -1;
    //     }
    // }

    s->set_vflip(s, 1);

    g_camera_initialized = true;
    g_current_resolution = resolution;
    g_current_format = format;
    return 0;
}

int camera_capture(captureMode mode, Image *inImg)
{
    (void)mode;
    if (inImg == NULL || inImg->pixels == NULL) {
        return -2;
    }

    camera_fb_t *fb = NULL;
    for (int attempt = 0; attempt < 3; ++attempt) {
        fb = esp_camera_fb_get();
        if (fb && fb->len > 0) {
            break;
        }
        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }
        delay(5);
    }
    if (!fb) {
        return -1;
    }

    const size_t expected_size =
        (size_t)inImg->width * (size_t)inImg->height * (size_t)inImg->depth;
    const bool is_wqvga_target = (inImg->width == 480u && inImg->height == 272u);
    const bool can_crop_hvga_to_wqvga =
        is_wqvga_target && (fb->width == 480u) && (fb->height == 320u);

    if (inImg->format == IMAGE_FORMAT_RGB565) {
        if (fb->format != PIXFORMAT_RGB565) {
            esp_camera_fb_return(fb);
            return -3;
        }

        if (can_crop_hvga_to_wqvga) {
            const size_t src_row_bytes = (size_t)fb->width * 2u;
            const size_t dst_row_bytes = (size_t)inImg->width * 2u;
            const size_t start_y = ((size_t)fb->height - (size_t)inImg->height) / 2u;
            const size_t needed = src_row_bytes * (size_t)fb->height;
            if (fb->len < needed) {
                esp_camera_fb_return(fb);
                return -3;
            }

            uint8_t *dst = (uint8_t *)inImg->pixels;
            const uint8_t *src = fb->buf + (start_y * src_row_bytes);
            for (size_t y = 0; y < inImg->height; ++y) {
                memcpy(dst + (y * dst_row_bytes), src + (y * src_row_bytes), dst_row_bytes);
            }
        } else {
            if (fb->width != inImg->width || fb->height != inImg->height ||
                fb->len < expected_size) {
                esp_camera_fb_return(fb);
                return -3;
            }
            memcpy(inImg->pixels, fb->buf, expected_size);
        }
    } else if (inImg->format == IMAGE_FORMAT_GRAYSCALE) {
        if (fb->format == PIXFORMAT_GRAYSCALE) {
            if (can_crop_hvga_to_wqvga) {
                const size_t src_row_bytes = (size_t)fb->width;
                const size_t dst_row_bytes = (size_t)inImg->width;
                const size_t start_y = ((size_t)fb->height - (size_t)inImg->height) / 2u;
                const size_t needed = src_row_bytes * (size_t)fb->height;
                if (fb->len < needed) {
                    esp_camera_fb_return(fb);
                    return -3;
                }

                uint8_t *dst = (uint8_t *)inImg->pixels;
                const uint8_t *src = fb->buf + (start_y * src_row_bytes);
                for (size_t y = 0; y < inImg->height; ++y) {
                    memcpy(dst + (y * dst_row_bytes), src + (y * src_row_bytes), dst_row_bytes);
                }
            } else {
                if (fb->width != inImg->width || fb->height != inImg->height ||
                    fb->len < expected_size) {
                    esp_camera_fb_return(fb);
                    return -3;
                }
                memcpy(inImg->pixels, fb->buf, expected_size);
            }
        } else if (fb->format == PIXFORMAT_YUV422) {
            const size_t src_row_bytes = (size_t)fb->width * 2u;
            const size_t start_y =
                can_crop_hvga_to_wqvga ? (((size_t)fb->height - (size_t)inImg->height) / 2u) : 0u;
            if ((!can_crop_hvga_to_wqvga &&
                 (fb->width != inImg->width || fb->height != inImg->height)) ||
                fb->len < (src_row_bytes * (size_t)fb->height)) {
                esp_camera_fb_return(fb);
                return -3;
            }
        } else {
            esp_camera_fb_return(fb);
            return -3;
        }
    } else {
        esp_camera_fb_return(fb);
        return -3;
    }

    esp_camera_fb_return(fb);

    return 0;
}

int camera_stop(void)
{
    return camera_deinit_internal();
}
int camera_setRes(ImageResolution resolution)
{
    if (g_camera_initialized && resolution == g_current_resolution) {
        return 0;
    }

    if (g_camera_initialized) {
        if (camera_deinit_internal() != 0) {
            return -1;
        }
    }

    return camera_init(resolution, g_current_format);
}

camera_t esp32_ov2640 = {.init = camera_init,
                         .capture = camera_capture,
                         .stop = camera_stop,
                         .setRes = camera_setRes};

#endif
