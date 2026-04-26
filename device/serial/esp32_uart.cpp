// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef DEVICE_ESP32_UART

    #include "core/error.h"
    #include "device/serial/serial.h"

    #include <stdarg.h>
    #include <stdio.h>

    #include "Arduino.h"
    #include "esp_camera.h"
    #define UART_BUF_SIZE 65535

const int RED_LED = 21;
const int WHT_LED = 22;

static void delay_ms(uint32_t ms)
{
    uint32_t startms = millis();
    while (millis() < (startms + ms)) {
        delay(1);
    }
}

int serial_init(void)
{
    Serial.flush();
    // Keep protocol channel clean (no ROM/SDK debug mixed into binary frames).
    Serial.setDebugOutput(false);
    Serial.begin(500000);
    Serial.setTimeout(30000);
    return EMBEDDIP_OK;
}

int serial_flush(void)
{
    while (Serial.available() > 0) {
        (void)Serial.read();
    }
    return EMBEDDIP_OK;
}

int serial_capture(Image *img)
{
    if (!img || !img->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Drop stale bytes from previous interrupted transaction.
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    uint8_t request_start_sequence[4] = "STR";

    uint32_t len = (uint32_t)img->height * (uint32_t)img->width * (uint32_t)img->depth;
    uint16_t block_size = (len < UART_BUF_SIZE) ? (uint16_t)len : UART_BUF_SIZE;
    uint32_t block_count = (block_size > 0) ? (len / block_size) : 0;
    uint16_t last_block_size = (block_size > 0) ? (uint16_t)(len % block_size) : 0;

    Serial.write(request_start_sequence, 3);

    uint32_t w = img->width;
    uint32_t h = img->height;
    uint32_t f = (img->format == IMAGE_FORMAT_MASK) ? IMAGE_FORMAT_GRAYSCALE : img->format;
    uint32_t d = img->depth;

    Serial.write((uint8_t *)&w, sizeof(w));
    Serial.write((uint8_t *)&h, sizeof(h));
    Serial.write((uint8_t *)&f, sizeof(f));
    Serial.write((uint8_t *)&d, sizeof(d));

    for (uint32_t i = 0; i < block_count; i++) {
        size_t got = Serial.readBytes(((uint8_t *)img->pixels) + (i * block_size), block_size);
        if (got != block_size) {
            return EMBEDDIP_ERROR_IO_ERROR;
        }
    }

    if (last_block_size) {
        size_t got = Serial.readBytes(((uint8_t *)img->pixels) + (block_count * block_size),
                                      last_block_size);
        if (got != last_block_size) {
            return EMBEDDIP_ERROR_IO_ERROR;
        }
    }

    return EMBEDDIP_OK;
}

int serial_send(const Image *img)
{
    if (!img || !img->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    uint8_t request_start_sequence[4] = "STW";
    uint32_t len = (uint32_t)img->width * (uint32_t)img->height * (uint32_t)img->depth;

    uint16_t block_size = (len < UART_BUF_SIZE) ? (uint16_t)len : UART_BUF_SIZE;
    uint32_t block_count = (block_size > 0) ? (len / block_size) : 0;
    uint16_t last_block_size = (block_size > 0) ? (uint16_t)(len % block_size) : 0;

    Serial.write(request_start_sequence, 3);
    delay_ms(1);

    uint32_t w = img->width;
    uint32_t h = img->height;
    uint32_t f = (img->format == IMAGE_FORMAT_MASK) ? IMAGE_FORMAT_GRAYSCALE : img->format;
    uint32_t d = img->depth;

    Serial.write((uint8_t *)&w, sizeof(w));
    Serial.flush();
    Serial.write((uint8_t *)&h, sizeof(h));
    Serial.flush();
    Serial.write((uint8_t *)&f, sizeof(f));
    Serial.flush();
    Serial.write((uint8_t *)&d, sizeof(d));
    Serial.flush();
    delay_ms(2);

    if (img->format == IMAGE_FORMAT_RGB565) {
        // Keep wire format stable for host decoders expecting RGB565 byte order.
        static uint8_t tx_swap_buf[1024];
        const uint8_t *src = (const uint8_t *)img->pixels;
        uint32_t remaining = len;

        while (remaining > 0) {
            uint16_t chunk = (remaining > sizeof(tx_swap_buf)) ? sizeof(tx_swap_buf) : remaining;
            if (chunk & 1u) {
                chunk--;
            }

            for (uint16_t j = 0; j < chunk; j += 2) {
                tx_swap_buf[j] = src[j + 1];
                tx_swap_buf[j + 1] = src[j];
            }
            Serial.write(tx_swap_buf, chunk);
            src += chunk;
            remaining -= chunk;
        }
    } else {
        for (uint32_t i = 0; i < block_count; i++) {
            Serial.write(((uint8_t *)img->pixels) + (i * block_size), block_size);
        }

        if (last_block_size) {
            Serial.write(((uint8_t *)img->pixels) + (block_count * block_size), last_block_size);
        }
    }

    delay_ms(2);
    return EMBEDDIP_OK;
}

int serial_send_jpeg(const Image *img)
{
    if (!img || !img->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    const uint8_t header[3] = {'S', 'T', 'J'};
    Serial.write(header, sizeof(header));

    uint32_t jpeg_size = img->size;
    Serial.write((uint8_t *)&jpeg_size, sizeof(jpeg_size));

    const uint8_t *ptr = (const uint8_t *)img->pixels;
    uint32_t remaining = jpeg_size;

    while (remaining > 0) {
        uint16_t chunk = (remaining > UART_BUF_SIZE) ? UART_BUF_SIZE : remaining;
        Serial.write(ptr, chunk);
        ptr += chunk;
        remaining -= chunk;
    }

    return EMBEDDIP_OK;
}

int serial_send_1d(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
{
    if (!data || elem_size == 0 || length == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    char header[3] = {'S', 'T', '0' + (char)type};
    Serial.write((uint8_t *)header, 3);

    Serial.write((uint8_t *)&length, sizeof(uint32_t));
    Serial.write(&elem_size, sizeof(uint8_t));
    delay_ms(1);

    const uint8_t *ptr = (const uint8_t *)data;
    uint32_t remaining = elem_size * length;

    while (remaining > 0) {
        uint16_t chunk = (remaining > UART_BUF_SIZE) ? UART_BUF_SIZE : remaining;
        Serial.write(ptr, chunk);
        ptr += chunk;
        remaining -= chunk;
        delay_ms(1);
    }

    return EMBEDDIP_OK;
}

serial_t esp32_uart = {
    .init = serial_init,
    .flush = serial_flush,
    .capture = serial_capture,
    .send = serial_send,
    .sendJPEG = serial_send_jpeg,
    .send1D = serial_send_1d,
};

#endif
