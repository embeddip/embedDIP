// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef SERIAL_H
#define SERIAL_H

#include "core/image.h"

#include <stddef.h>
#include <stdint.h>

#include "embedDIP_configs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SERIAL_DATA_HISTOGRAM = 1,       // "ST1"
    SERIAL_DATA_PROJECTION = 2,      // "ST2"
    SERIAL_DATA_LINE_PROFILE = 3,    // "ST3"
    SERIAL_DATA_KERNEL = 4,          // "ST4"
    SERIAL_DATA_TEMPORAL_TRACE = 5,  // "ST5"
    SERIAL_DATA_GRADIENT = 6,        // "ST6"
    SERIAL_DATA_OTHER = 7            // "ST7" or use dynamically
} Serial1DDataType;

typedef struct serial_interface {
    int (*init)(void);
    int (*flush)(void);  // not known.
    int (*capture)(Image *img);
    int (*send)(const Image *img);
    int (*sendJPEG)(const Image *img);
    int (*send1D)(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type);
} serial_t;

int _write(int file, char *ptr, int len);

// External declaration of STM32 implementation
#ifdef EMBED_DIP_BOARD_STM32F7
extern serial_t stm32_uart;
#endif

#ifdef EMBED_DIP_BOARD_ESP32
extern serial_t esp32_uart;
#endif

#ifdef __cplusplus
}
#endif

#endif  // SERIAL_H
