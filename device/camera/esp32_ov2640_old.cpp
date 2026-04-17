// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

// This is an alternative OV2640 implementation. Use esp32_ov2640.cpp instead.
// To enable this implementation, define DEVICE_OV2640_ALT in your project.
#if defined(DEVICE_OV2640_ALT)

    #include "device/camera/camera.h"

    #include <string.h>

    #include "Arduino.h"
    #include "esp32-hal-ledc.h"
    #include "esp_camera.h"
    #include <esp_log.h>
    #include <esp_system.h>
    #include <nvs_flash.h>
    #include <sys/param.h>

    #define CAMERA_MODEL_ESP_EYE  // Has PSRAM
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

    #define LED_GPIO_NUM 22
void setupLedFlash()
{
    #if defined(LED_GPIO_NUM)
    ledcAttach(LED_GPIO_NUM, 5000, 8);
    #else
    log_i("LED flash is disabled -> LED_GPIO_NUM undefined");
    #endif
}
int camera_init(ImageResolution resolution, ImageFormat format)
{
    Serial.println("[INFO] Starting camera initialization...");

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
    config.xclk_freq_hz = 10000000;
    config.frame_size = (framesize_t)resolution;
    config.pixel_format = (format == IMAGE_FORMAT_RGB565) ? PIXFORMAT_RGB565 : PIXFORMAT_GRAYSCALE;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // Serial.println("[INFO] Camera pins and default config set.");

    if (config.pixel_format == PIXFORMAT_JPEG) {
        Serial.println("[INFO] JPEG mode detected.");
        if (psramFound()) {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
            // Serial.println("[INFO] PSRAM detected. Using higher quality JPEG and double
            // buffers.");
        } else {
            config.frame_size = (framesize_t)resolution;
            config.fb_location = CAMERA_FB_IN_DRAM;
            // Serial.println("[WARN] PSRAM not found. Lowering resolution and using DRAM.");
        }
    } else {
        config.frame_size = (framesize_t)resolution;
    #if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
    #endif
        // Serial.println("[INFO] Non-JPEG mode. Using QQVGA and double buffer (if ESP32S3).");
    }

    #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
        // Serial.println("[INFO] Pull-up enabled on ESP-EYE buttons.");
    #endif

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[ERROR] Camera init failed with error 0x%x\n", err);
        return -1;
    }
    // Serial.println("[INFO] Camera initialized successfully.");

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        // Serial.println("[INFO] Detected sensor OV3660. Applying default tuning...");
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }

    if (config.pixel_format == PIXFORMAT_JPEG) {
        // Serial.println("[INFO] Setting JPEG frame size to QVGA.");
        s->set_framesize(s, FRAMESIZE_QVGA);
    }

    #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
        // Serial.println("[INFO] M5STACK: Flipping and mirroring image.");
    #endif

    #if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
        // Serial.println("[INFO] ESP32S3_EYE: Applying vertical flip.");
    #endif

    #if defined(LED_GPIO_NUM)
    setupLedFlash();
        // Serial.println("[INFO] LED Flash setup completed.");
    #endif

    // Serial.println("[INFO] Camera setup completed.");
    return 0;
}

int camera_capture(captureMode mode, Image *inImg)
{
    // Serial.println("[INFO] Capturing image from camera...");

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        // Serial.println("[ERROR] Failed to get frame buffer from camera.");
        return -1;
    }

    // Serial.printf("[INFO] Frame captured. Resolution: %dx%d, Format: %d, Size: %d bytes\n",
    // fb->width, fb->height, fb->format, fb->len);

    if (inImg == NULL || inImg->pixels == NULL) {
        // Serial.println("[ERROR] Output image buffer is NULL.");
        esp_camera_fb_return(fb);
        return -2;
    }

    if (fb->width != inImg->width || fb->height != inImg->height) {
        // Serial.printf("[ERROR] Size mismatch! Expected: %dx%d, Got: %dx%d\n", inImg->width,
        // inImg->height, fb->width, fb->height);
        esp_camera_fb_return(fb);
        return -3;
    }

    if (fb->format != PIXFORMAT_GRAYSCALE) {
        // Serial.printf("[ERROR] Pixel format mismatch. Expected GRAYSCALE (1), got: %d\n",
        // fb->format);
        esp_camera_fb_return(fb);
        return -4;
    }

    memcpy(inImg->pixels, fb->buf, fb->len);
    // Serial.printf("[INFO] Image copied to buffer. %d bytes written.\n", fb->len);

    esp_camera_fb_return(fb);
    // Serial.println("[INFO] Frame buffer returned to camera driver.");

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
