// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef DEVICE_OV2640

    #include "device/camera/camera.h"

    #include <string.h>

    #include "Arduino.h"
    #include "esp32-hal-ledc.h"
    #include "esp_camera.h"
    #include <esp_log.h>
    #include <esp_system.h>
    #include <nvs_flash.h>
    #include <sys/param.h>

    // Select your camera module - uncomment ONE of these
    // #define CAMERA_MODEL_AI_THINKER  // Most common ESP32-CAM
    #define CAMERA_MODEL_ESP_EYE

    #ifdef CAMERA_MODEL_AI_THINKER
        #define PWDN_GPIO_NUM 32
        #define RESET_GPIO_NUM -1
        #define XCLK_GPIO_NUM 0
        #define SIOD_GPIO_NUM 26
        #define SIOC_GPIO_NUM 27
        #define Y9_GPIO_NUM 35
        #define Y8_GPIO_NUM 34
        #define Y7_GPIO_NUM 39
        #define Y6_GPIO_NUM 36
        #define Y5_GPIO_NUM 21
        #define Y4_GPIO_NUM 19
        #define Y3_GPIO_NUM 18
        #define Y2_GPIO_NUM 5
        #define VSYNC_GPIO_NUM 25
        #define HREF_GPIO_NUM 23
        #define PCLK_GPIO_NUM 22
    #elif defined(CAMERA_MODEL_ESP_EYE)
        #define PWDN_GPIO_NUM -1
        #define RESET_GPIO_NUM -1
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
    #endif

    /*

    typedef enum {
        FRAMESIZE_96X96,    // 96x96
        FRAMESIZE_QQVGA,    // 160x120
        FRAMESIZE_128X128,    // 128x128
        FRAMESIZE_QCIF,     // 176x144
        FRAMESIZE_HQVGA,    // 240x176
        FRAMESIZE_240X240,  // 240x240
        FRAMESIZE_QVGA,     // 320x240
        FRAMESIZE_320X320,  // 320x320
        FRAMESIZE_CIF,      // 400x296
        FRAMESIZE_HVGA,     // 480x320
        FRAMESIZE_VGA,      // 640x480
        FRAMESIZE_SVGA,     // 800x600
        FRAMESIZE_XGA,      // 1024x768
        FRAMESIZE_HD,       // 1280x720
        FRAMESIZE_SXGA,     // 1280x1024
        FRAMESIZE_UXGA,     // 1600x1200
        // 3MP Sensors
        FRAMESIZE_FHD,      // 1920x1080
        FRAMESIZE_P_HD,     //  720x1280
        FRAMESIZE_P_3MP,    //  864x1536
        FRAMESIZE_QXGA,     // 2048x1536
        // 5MP Sensors
        FRAMESIZE_QHD,      // 2560x1440
        FRAMESIZE_WQXGA,    // 2560x1600
        FRAMESIZE_P_FHD,    // 1080x1920
        FRAMESIZE_QSXGA,    // 2560x1920
        FRAMESIZE_5MP,      // 2592x1944
        FRAMESIZE_INVALID
    } framesize_t;

    */
    #define LED_GPIO_NUM 22
void setupLedFlash()
{
    #if defined(LED_GPIO_NUM)
        // ledcAttach(LED_GPIO_NUM, 5000, 8);
    #else
    log_i("LED flash is disabled -> LED_GPIO_NUM undefined");
    #endif
}

// Map embedDIP ImageResolution to esp32-camera framesize_t
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
        return FRAMESIZE_QVGA;  // 480x272 -> use 320x240 as closest
    default:
        return FRAMESIZE_QVGA;
    }
}

int camera_init(ImageResolution resolution, ImageFormat format)
{
    camera_config_t config;
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
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = map_resolution(resolution);
    config.pixel_format =
        (format == IMAGE_FORMAT_GRAYSCALE) ? PIXFORMAT_GRAYSCALE : PIXFORMAT_RGB565;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 1;

    #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
    #endif

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        return -1;
    }

    sensor_t *s = esp_camera_sensor_get();

    // Explicitly set pixel format on sensor
    pixformat_t pix_fmt =
        (format == IMAGE_FORMAT_GRAYSCALE) ? PIXFORMAT_GRAYSCALE : PIXFORMAT_RGB565;
    if (s->set_pixformat) {
        s->set_pixformat(s, pix_fmt);
    }

    // Explicitly set frame size on sensor
    if (s->set_framesize) {
        s->set_framesize(s, map_resolution(resolution));
    }

    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }

    #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    #endif

    #if defined(CAMERA_MODEL_ESP32S3_EYE) || defined(CAMERA_MODEL_ESP_EYE)
    s->set_vflip(s, 1);
    #endif

    return 0;
}

int camera_capture(captureMode mode, Image *inImg)
{
    if (inImg == NULL || inImg->pixels == NULL) {
        return -2;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        return -1;
    }

    // Serial.printf("[INFO] Frame captured. Resolution: %dx%d, Format: %d, Size: %d bytes\n",
    // fb->width, fb->height, fb->format, fb->len);

    if (inImg == NULL || inImg->pixels == NULL) {
        // Serial.println("[ERROR] Output image buffer is NULL.");
        esp_camera_fb_return(fb);
        return -2;
    }

    // Check if image size matches (allow some flexibility for data size)
    size_t expected_size = inImg->width * inImg->height * inImg->depth;
    if (fb->len != expected_size) {
        // Serial.printf("[WARN] Size mismatch! Expected: %u bytes, Got: %u bytes\n", expected_size,
        // fb->len);
        //  Continue anyway - might still work
    }

    // Copy frame buffer data to image buffer
    size_t copy_size = (fb->len < expected_size) ? fb->len : expected_size;
    memcpy(inImg->pixels, fb->buf, copy_size);

    // Convert RGB565 to BGR565 (swap red and blue channels)
    if (inImg->format == IMAGE_FORMAT_RGB565 && inImg->depth == 2) {
        uint16_t *pixels = (uint16_t *)inImg->pixels;
        size_t num_pixels = inImg->width * inImg->height;

        for (size_t i = 0; i < num_pixels; i++) {
            uint16_t rgb = pixels[i];
            // Extract R, G, B components
            uint16_t r = (rgb >> 11) & 0x1F;  // Red: bits 11-15
            uint16_t g = (rgb >> 5) & 0x3F;   // Green: bits 5-10
            uint16_t b = (rgb >> 0) & 0x1F;   // Blue: bits 0-4
            // Repack as BGR565
            pixels[i] = (b << 11) | (g << 5) | r;
        }
    }

    esp_camera_fb_return(fb);

    return 0;
}

int camera_stop(void)
{
    return 0;
}
int camera_setRes(ImageResolution resolution)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s)
        return -1;

    s->set_framesize(s, (framesize_t)resolution);

    return 0;
}

camera_t esp32_ov2640 = {.init = camera_init,
                         .capture = camera_capture,
                         .stop = camera_stop,
                         .setRes = camera_setRes};

#endif
